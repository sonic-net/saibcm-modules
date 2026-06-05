/*! \file ngknet_adapter.c
 *
 * Adapter routines for Linux kernel APIs abstraction.
 *
 * RX: From CNET ...
 *   ngknet_dev_pkt_recv() : See bcmxxx_xx_pdma_attach.c : *pdev->pkt_recv()*
 *         |
 *         |-> ngknet_adapter_frame_recv()
 *         |         |
 *         |         \-> ngknet_rx_parser() : Parser packet and fill the scratch data.
 *         |                   |
 *         |                   \-> skb_put(scratch_data) : 16 Bytes
 *         |
 *         \-> ngknet_frame_recv()
 *                   |
 *                   |-> ngknet_rx_pkt_filter()
 *                   |         |
 *                   |         |-> ngknet_filter_match() : if true
 *                   |         |         |
 *                   |         |         |-> ngknet_adapter_filter_scratch_data_match() : (DNX devices only)
 *                   |         |         |
 *                   |         |         \-> Compare the OOB and scratch data.
 *                   |         |
 *                   |         |-> ngknet_filter_callback()
 *                   |         |
 *                   |         \-> ngknet_filter_process()
 *                   |                   |
 *                   |                   \-> _adapter_pkt_recv() : *priv->pkt_recv()*
 *                   |                               |
 *                   |                               |-> skb_trim(scratch_data) : 16 Bytes
 *                   |                               |
 *                   |                               \-> ngknet_pkt_recv()
 *                   |                                         |
 *                   |                                         \-> ngknet_netif_recv()
 *                   |                                                   |
 *                   |                                                   |-> ngknet_rx_frame_process()
 *                   |                                                   |         |
 *                   |                                                   |         |-> Add RCPU encapsulation or strip matadata if needed.
 *                   |                                                   |         |
 *                   |                                                   |         \-> rx_cb()
 *                   |                                                   |
 *                   |                                                   \-> netif_receive_skb() : to netif.
 *                   |
 *                   \-> ngknet_pkt_stats()
 *
 * TX: To CNET ...
 *   ngknet_start_xmit() : from netif.
 *         |
 *         |-> ngknet_pkt_stats()
 *         |
 *         |-> ngknet_tx_frame_process()
 *         |         |
 *         |         |-> Strip RCPU encapsulation, setup CNET packet buffer, add vlan tag.
 *         |         |
 *         |         \-> tx_cb()
 *         |
 *         |-> ngknet_tx_queue_schedule()
 *         |
 *         |-> skb_tx_timestamp()
 *         |
 *         \-> ngknet_adapter_frame_xmit() : *pdev->pkt_xmit()*
 *                   |
 *                   \-> pdev->pkt_xmit()
 *
 */
/*
 *
 * $Copyright: 2017-2026 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

#include <linux/spinlock.h>

#include <lkm/ngknet_kapi.h>

#include "ngknet_adapter.h"
#include "ngknet_parser.h"

extern struct ngknet_dev ngknet_devices[];

static ibde_t *kernel_bde = NULL;
static int debug = 0;

static struct adapter_dev adapter_devices[NUM_PDMA_DEV_MAX] = {0};

#define ADAPTER_RX_START_INFO(_adev)              ((_adev)->rsi)
#define ADAPTER_RX_START_INFO_IS_DELIVERED(_adev) (ADAPTER_RX_START_INFO(_adev).flags & NGKNET_RX_START_INFO_DELIVERED)

/* CMICR interrupts reserved for kernel handler */
#define CMICR_TXRX_IRQ_MASK     0xffff00
/* CMICx interrupts reserved for kernel handler */
#define CMICX_TXRX_IRQ_MASK     0xffffffff

/*
 * PAXB_0_INTC_SET_INTR_ENABLE_REG5r sets interrupt enable bit for interrupts 191 down to 160
 * Packet DMA interrupt enable bit is [168 : 199], here bit [168 : 183] is considered because it is assumed that only cmc0
 * PAXB_0_INTC_INTR_ENABLE_STATUS_REG5 is used to check the intr enabled status.
 * PAXB_0_INTC_CLEAR_INTR_ENABLE_REG5 is used to disable the intrs.
 */
#define PAXB_0_INTC_SET_INTR_ENABLE_REG5r   0x0292D114

#define CMICX_IRQ_ENABr         0x18013100

/*
 * Get a 16-bit value from packet offset
 * _data Pointer to packet
 * _offset Offset
 */
#define PKT_U16_GET(_data, _offset) \
    (uint16_t)(_data[_offset] << 8 | _data[_offset + 1])

static void
_adapter_pkt_recv(struct net_device *ndev, struct sk_buff *skb);

static int
_adapter_info_show(
    struct seq_file *m,
    void *v)
{
    struct ngknet_dev *dev;
    struct adapter_dev *adev;
    int di, ai = 0, gi, qi, chan_id, i, j;
    int queue, dir;
    int hw_grp, hw_que;
    char *cmic_type_str[] = ADAPTER_CMIC_TYPE_STR;

    for (di = 0; di < NUM_PDMA_DEV_MAX; di++) {
        struct pdma_dev *pdev = NULL;
        struct pdma_hw *phw = NULL;
        struct net_device *ndev = NULL;
        struct ngknet_private *priv = NULL;

        adev = &adapter_devices[di];

        if (!(adev->flags & ADAPTER_ACTIVE)) {
            continue;
        }
        ai++;
        pdev = adev->pdma_dev;
        phw  = adev->pdma_hw;
        dev  = adev->dev;

        seq_printf(m, "kdev: %d\n",     di);
        seq_printf(m, "------------ dev -------------\n");
        seq_printf(m, "device_is_sand(%d): %s\n", di, device_is_sand(di) ? "Yes" : "No");
        seq_printf(m, "------- dev->dev_info --------\n");
        seq_printf(m, "dev_no:           %d\n",   dev->dev_info.dev_no);
        seq_printf(m, "dev_id:           0x%x\n", dev->dev_info.dev_id);
        seq_printf(m, "type_str:         %s\n",   dev->dev_info.type_str);
        seq_printf(m, "var_str:          %s\n",   dev->dev_info.var_str);
        seq_printf(m, "------- dev->rcpu_ctrl -------\n");
        seq_printf(m, "dst_mac:          ");
        for (i = 0; i < 6; i++) {
            seq_printf(m, "%02x%s", dev->rcpu_ctrl.dst_mac[i], i == (6-1) ? "\n":":");
        }
        seq_printf(m, "src_mac:          ");
        for (i = 0; i < 6; i++) {
            seq_printf(m, "%02x%s", dev->rcpu_ctrl.src_mac[i], i == (6-1) ? "\n":":");
        }
        seq_printf(m, "vlan_tpid:        0x%04x (0x%04x)\n", dev->rcpu_ctrl.vlan_tpid, htons(dev->rcpu_ctrl.vlan_tpid));
        seq_printf(m, "vlan_tci:         0x%04x (0x%04x)\n", dev->rcpu_ctrl.vlan_tci,  htons(dev->rcpu_ctrl.vlan_tci));
        seq_printf(m, "eth_type:         0x%04x (0x%04x)\n", dev->rcpu_ctrl.eth_type,  htons(dev->rcpu_ctrl.eth_type));
        seq_printf(m, "pkt_sig:          0x%04x (0x%04x)\n", dev->rcpu_ctrl.pkt_sig,   htons(dev->rcpu_ctrl.pkt_sig));
        seq_printf(m, "trans_id:         0x%04x (0x%04x)\n", dev->rcpu_ctrl.trans_id,  htons(dev->rcpu_ctrl.trans_id));
        seq_printf(m, "------- dev->net_dev ----------\n");
        ndev = dev->net_dev;
        if (ndev) {
            priv = netdev_priv(ndev);
            seq_printf(m, "net_dev:\n");
            seq_printf(m, "  priv:\n");
            seq_printf(m, "    netif.id:     %d\n",     priv->netif.id);
            seq_printf(m, "    netif.flags:  0x%x%s\n", priv->netif.flags, priv->netif.flags & NGKNET_NETIF_F_USE_SHARED_NDEV ? "(shared)":"");
            seq_printf(m, "    pkt_recv:     %s\n",     priv->pkt_recv == _adapter_pkt_recv ? "_adapter_pkt_recv": priv->pkt_recv ? "ngknet_pkt_recv":"");
            seq_printf(m, "    users:        %d\n",     priv->users);
            seq_printf(m, "    ref_count:    %d\n",     priv->ref_count);
        }
        seq_printf(m, "------- dev->bdev[?] ---------\n");
        for (chan_id = 0; chan_id < NUM_Q_MAX; chan_id++) {
            ndev = dev->bdev[chan_id];
            if (ndev) {
                priv = netdev_priv(ndev);
                seq_printf(m, "chan_id:          %d\n",     chan_id);
                seq_printf(m, "  priv:\n");
                seq_printf(m, "    netif.id:     %d\n",     priv->netif.id);
                seq_printf(m, "    netif.flags:  0x%x%s\n", priv->netif.flags, priv->netif.flags & NGKNET_NETIF_F_USE_SHARED_NDEV ? "(shared)":"");
                seq_printf(m, "    bkn_dev:      %s\n",     priv->bkn_dev == dev ? "Master":"Slave");
                seq_printf(m, "    pkt_recv:     %s\n",     priv->pkt_recv == _adapter_pkt_recv ? "_adapter_pkt_recv": priv->pkt_recv ? "ngknet_pkt_recv":"");
                seq_printf(m, "    users:        %d\n",     priv->users);
                seq_printf(m, "    ref_count:    %d\n",     priv->ref_count);
            }
        }
        seq_printf(m, "------- dev->vdev[?] ---------\n");
        seq_printf(m, "num:              %ld\n",     (long)dev->vdev[0]);
        for (i = 1; i < NUM_VDEV_MAX + 1; i++) {
            ndev = dev->vdev[i];
            if (ndev) {
                priv = netdev_priv(ndev);
                seq_printf(m, "netif_id:         %d\n",     i);
                seq_printf(m, "  priv:\n");
                seq_printf(m, "    netif.id:     %d\n",     priv->netif.id);
                seq_printf(m, "    netif.flags:  0x%x%s\n", priv->netif.flags, priv->netif.flags & NGKNET_NETIF_F_USE_SHARED_NDEV ? "(shared)":"");
                seq_printf(m, "    bkn_dev:      %s\n",     priv->bkn_dev == dev ? "Master":"Slave");
                seq_printf(m, "    pkt_recv:     %s\n",     priv->pkt_recv == _adapter_pkt_recv ? "_adapter_pkt_recv": priv->pkt_recv ? "ngknet_pkt_recv":"");
                seq_printf(m, "    users:        %d\n",     priv->users);
                seq_printf(m, "    ref_count:    %d\n",     priv->ref_count);
            }
        }
        seq_printf(m, "------------ adev ------------\n");
        seq_printf(m, "debug:            0x%x\n",   debug);
        seq_printf(m, "base_addr:        %p\n",     ngbde_kapi_pio_membase(di));
        seq_printf(m, "cmic_type:        %s\n",     cmic_type_str[adev->cmic_type]);
        seq_printf(m, "nof_chans:        %d\n",     adev->nof_chans);
        seq_printf(m, "irq_mask:         0x%x\n",   adev->irq_mask);
        seq_printf(m, "irq_mask_reg:     0x%x\n",   adev->irq_mask_reg);
        seq_printf(m, "irq_fmask:        0x%x\n",   adev->irq_fmask);
        seq_printf(m, "irq_status_reg:   0x%x\n",   adev->irq_status_reg);
        seq_printf(m, "irq_mask_cnt:     %d\n",     adev->irq_mask_cnt);
        seq_printf(m, "irq_num:          %d\n",     adev->irq_num);
        seq_printf(m, "isr_connected:    %d\n",     adev->isr_connected);
        seq_printf(m, "isr_received:     %d\n",     adev->isr_received);
        seq_printf(m, "isr_handled:      %d\n",     adev->isr_handled);
#if ADAPTER_DMA_ADDR_DEBUG
        seq_printf(m, "p2l_cnt:          %d\n",     adev->p2l_cnt);
        seq_printf(m, "p2l_baddr:        0x%llx (dma_addr_t %lu)\n",  adev->p2l_baddr, sizeof(dma_addr_t));
        seq_printf(m, "p2l_paddr:        0x%lx (sal_paddr_t %lu)\n",  adev->p2l_paddr, sizeof(sal_paddr_t));
        seq_printf(m, "p2l_vaddr:        %p (void * %lu)\n",          adev->p2l_vaddr, sizeof(void *));
        seq_printf(m, "l2p_cnt:          %d\n",     adev->l2p_cnt);
        seq_printf(m, "l2p_vaddr:        %p (void * %lu)\n",          adev->l2p_vaddr, sizeof(void *));
        seq_printf(m, "l2p_paddr:        0x%lx (sal_paddr_t %lu)\n",  adev->l2p_paddr, sizeof(sal_paddr_t));
        seq_printf(m, "l2p_baddr:        0x%llx (dma_addr_t %lu)\n",  adev->l2p_baddr, sizeof(dma_addr_t));
#endif /* ADAPTER_DMA_ADDR_DEBUG */
        seq_printf(m, "iio_base:         0x%x\n",   adev->iio_base);
        seq_printf(m, "iio_recorder_cnt: %d\n",     adev->iio_recorder_cnt);
        for (i = 0; i < ADAPTER_IIO_RECORDER_SIZE; i++) {
            if (adev->iio_recorder[i].op) {
                char *op = adev->iio_recorder[i].op == ADAPTER_IIO_OP_WRITE ? "write" :
                           adev->iio_recorder[i].op == ADAPTER_IIO_OP_READ ? "read" : "none";
                bool curr = ((adev->iio_recorder_cnt - 1) % ADAPTER_IIO_RECORDER_SIZE) == i;
                seq_printf(m, "iio_recorder[%2d]: %s: [0x%x] = 0x%08x %s\n",
                           i, op, adev->iio_recorder[i].offs, adev->iio_recorder[i].val, curr ? "<--" : "");
            }
        }
        seq_printf(m, "imask_recorder_cnt: %d\n",     adev->imask_recorder_cnt);
        for (i = 0; i < ADAPTER_IMASK_RECORDER_SIZE; i++) {
            if (adev->imask_recorder[i].mask_val) {
                bool curr = ((adev->imask_recorder_cnt - 1) % ADAPTER_IMASK_RECORDER_SIZE) == i;
                seq_printf(m, "imask_recorder[%2d]: [%d] = 0x%08x %s\n",
                           i, adev->imask_recorder[i].inum, adev->imask_recorder[i].mask_val, curr ? "<--" : "");
            }
        }
        seq_printf(m, "def_netif:        %d\n",   adev->def_netif);
        seq_printf(m, "pkt_recv:         %s\n",   adev->pkt_recv == _adapter_pkt_recv ? "_adapter_pkt_recv" : adev->pkt_recv ? "ngknet_pkt_recv":"");
        seq_printf(m, "------------ rsi -------------\n");
        seq_printf(m, "flags:                       0x%x\n",   ADAPTER_RX_START_INFO(adev).flags);
        seq_printf(m, "enet_channels:               0x%x\n",   ADAPTER_RX_START_INFO(adev).enet_channels);
        seq_printf(m, "system_headers_mode:         0x%x\n",   ADAPTER_RX_START_INFO(adev).system_headers_mode);
        seq_printf(m, "ftmh_lb_key_size:            %d\n",     ADAPTER_RX_START_INFO(adev).ftmh_lb_key_size);
        seq_printf(m, "ftmh_stacking_ext_size:      %d\n",     ADAPTER_RX_START_INFO(adev).ftmh_stacking_ext_size);
        seq_printf(m, "pph_base_size:               %d\n",     ADAPTER_RX_START_INFO(adev).pph_base_size);
        for (i = 0; i < 8; i++) {
            seq_printf(m, "pph_lif_ext_size[%d]:         %d\n",i, ADAPTER_RX_START_INFO(adev).pph_lif_ext_size[i]);
        }
        seq_printf(m, "udh_enabled:                 %d\n",     ADAPTER_RX_START_INFO(adev).udh_enabled);
        for (i = 0; i < 4; i++) {
            seq_printf(m, "udh_length_type[%d]:          %d\n",i, ADAPTER_RX_START_INFO(adev).udh_length_type[i]);
        }
        seq_printf(m, "oamp_system_port_0:          %d\n",     ADAPTER_RX_START_INFO(adev).oamp_system_port_0);
        seq_printf(m, "oamp_system_port_1:          %d\n",     ADAPTER_RX_START_INFO(adev).oamp_system_port_1);
        seq_printf(m, "up_mep_ingress_cpu_trap_id1: %d\n",     ADAPTER_RX_START_INFO(adev).up_mep_ingress_cpu_trap_id1);
        seq_printf(m, "up_mep_ingress_cpu_trap_id2: %d\n",     ADAPTER_RX_START_INFO(adev).up_mep_ingress_cpu_trap_id2);
        seq_printf(m, "oamp_port_number:            %d {",     ADAPTER_RX_START_INFO(adev).oamp_port_number);
        for (i = 0; i < NGKNET_OAMP_PORT_MAX; i++) {
            seq_printf(m, "%d%s", ADAPTER_RX_START_INFO(adev).oamp_ports[i], i == (NGKNET_OAMP_PORT_MAX-1) ? "}\n":",");
        }
        seq_printf(m, "spa_mode:                    %d\n",     ADAPTER_RX_START_INFO(adev).spa_mode);
        seq_printf(m, "------- filter scratch -------\n");
        for (i = 1; i < NUM_FILTER_MAX; i++) {
            if (adev->scratch[i].filter_id == i) {
                seq_printf(m, "filter_id:        %d\n",     adev->scratch[i].filter_id);
                seq_printf(m, " spa_unit:        %d\n",     adev->scratch[i].spa_unit);
                seq_printf(m, "   mflags:        0x%x\n",   adev->scratch[i].match_flags);
                seq_printf(m, "     data:        ");
                for (j = 0; j < NGKNET_SCRATCH_DATA_WORDS; j++) {
                    seq_printf(m, "%08x%s", adev->scratch[i].data[j], j == (NGKNET_SCRATCH_DATA_WORDS-1) ? "\n":" ");
                }
                seq_printf(m, "     mask:        ");
                for (j = 0; j < NGKNET_SCRATCH_DATA_WORDS; j++) {
                    seq_printf(m, "%08x%s", adev->scratch[i].mask[j], j == (NGKNET_SCRATCH_DATA_WORDS-1) ? "\n":" ");
                }
            }
        }
        seq_printf(m, "------------ pdev ------------\n");
        seq_printf(m, "dev_id:           0x%x\n",   pdev->dev_id);
        seq_printf(m, "dev_type:         %d\n",     pdev->dev_type);
        seq_printf(m, "flags:            0x%x\n",   pdev->flags);
        seq_printf(m, "mode:             %d\n",     pdev->mode);
        seq_printf(m, "ops:              %s\n",     pdev->ops ? "Attached" : "Not Attach");
        seq_printf(m, "num_groups:       %d\n",     pdev->num_groups);
        seq_printf(m, "grp_queues:       %d\n",     pdev->grp_queues);
        seq_printf(m, "rx_ph_size:       %d\n",     pdev->rx_ph_size);
        seq_printf(m, "tx_ph_size:       %d\n",     pdev->tx_ph_size);
        seq_printf(m, "ctrl.bm_grp:      0x%x\n",   pdev->ctrl.bm_grp);
        seq_printf(m, "ctrl.nb_grp:      %d\n",     pdev->ctrl.nb_grp);
        seq_printf(m, "ctrl.bm_txq:      0x%x\n",   pdev->ctrl.bm_txq);
        seq_printf(m, "ctrl.nb_txq:      %d\n",     pdev->ctrl.nb_txq);
        seq_printf(m, "ctrl.bm_rxq:      0x%x\n",   pdev->ctrl.bm_rxq);
        seq_printf(m, "ctrl.nb_rxq:      %d\n",     pdev->ctrl.nb_rxq);

        for (gi = 0; gi < pdev->num_groups; gi++) {
            seq_printf(m, "  gi %d:           %s\n",    gi, pdev->ctrl.grp[gi].attached ? "Attached" : "");
            seq_printf(m, "    irq_mask:        0x%x\n",    pdev->ctrl.grp[gi].irq_mask);
            seq_printf(m, "    bm_rxq:          0x%x\n",    pdev->ctrl.grp[gi].bm_rxq);
            for (qi = 0; qi < pdev->grp_queues; qi++) {
                if (1 << qi & pdev->ctrl.grp[gi].bm_rxq) {
                    struct pdma_rx_queue *rxq = NULL;
                    struct intr_handle *ih = NULL;
                    pdev->ops->dev_pq_to_lq(pdev, qi + gi * pdev->grp_queues, &queue, &dir);
                    seq_printf(m, "      qi %d:\n",     qi);
                    seq_printf(m, "        queue:       %d\n",     queue);
                    seq_printf(m, "        dir:         %d\n",     dir);
                    rxq = (struct pdma_rx_queue *)pdev->ctrl.rx_queue[queue];
                    hw_grp = rxq->chan_id / adev->nof_chans;
                    hw_que = rxq->chan_id % adev->nof_chans;
                    seq_printf(m, "        chan_id:     %d\n",     rxq->chan_id);
                    seq_printf(m, "          hw_grp:    %d\n",     hw_grp);
                    seq_printf(m, "          hw_que:    %d\n",     hw_que);
                    seq_printf(m, "          irq_mask:  0x%x\n",   phw->dev->ctrl.grp[hw_grp].irq_mask);
                    ih = &pdev->ctrl.grp[gi].intr_hdl[qi];
                    seq_printf(m, "        intr_hdl:    group %d chan %d queue %d dir %d budget %d inum %d\n",
                               ih->group, ih->chan, ih->queue, ih->dir, ih->budget, ih->inum);
                    seq_printf(m, "                     intr_flags 0x%x extra_poll %s\n",
                               ih->intr_flags, ih->extra_poll ? "T":"F");
                }
            }
            seq_printf(m, "    bm_txq:          0x%x\n",    pdev->ctrl.grp[gi].bm_txq);
            for (qi = 0; qi < pdev->grp_queues; qi++) {
                if (1 << qi & pdev->ctrl.grp[gi].bm_txq) {
                    struct pdma_tx_queue *txq = NULL;
                    struct intr_handle *ih = NULL;
                    pdev->ops->dev_pq_to_lq(pdev, qi + gi * pdev->grp_queues, &queue, &dir);
                    seq_printf(m, "      qi %d:\n",     qi);
                    seq_printf(m, "        queue:       %d\n",     queue);
                    seq_printf(m, "        dir:         %d\n",     dir);
                    txq = (struct pdma_tx_queue *)pdev->ctrl.tx_queue[queue];
                    hw_grp = txq->chan_id / adev->nof_chans;
                    hw_que = txq->chan_id % adev->nof_chans;
                    seq_printf(m, "        chan_id:     %d\n",     txq->chan_id);
                    seq_printf(m, "          hw_grp:    %d\n",     hw_grp);
                    seq_printf(m, "          hw_que:    %d\n",     hw_que);
                    seq_printf(m, "          irq_mask:  0x%x\n",   phw->dev->ctrl.grp[hw_grp].irq_mask);
                    ih = &pdev->ctrl.grp[gi].intr_hdl[qi];
                    seq_printf(m, "        intr_hdl:    group %d chan %d queue %d dir %d budget %d inum %d\n",
                               ih->group, ih->chan, ih->queue, ih->dir, ih->budget, ih->inum);
                    seq_printf(m, "                     intr_flags 0x%x extra_poll %s\n",
                               ih->intr_flags, ih->extra_poll ? "T":"F");
                }
            }
        }
        seq_printf(m, "------------ phw -------------\n");
        seq_printf(m, "dev->mode:        %d\n",     phw->dev->mode);
    }
    if (!ai) {
        seq_printf(m, "%s\n", "No active device");
    } else {
        seq_printf(m, "------------------------\n");
        seq_printf(m, "Total %d devices\n", ai);
    }
    return 0;
}

static int
_adapter_info_open(
    struct inode *inode,
    struct file *file)
{
    return single_open(file, _adapter_info_show, NULL);
}

static int
_adapter_info_release(
    struct inode *inode,
    struct file *file)
{
    return single_release(inode, file);
}

struct proc_ops ngknet_adapter_info_fops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        _adapter_info_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     _adapter_info_release,
};

/*!
 * Dump packet content for debug
 */
static void
_adapter_pkt_dump(
    uint8_t *data,
    int len)
{
    char str[128];
    int i;

    len = len > 256 ? 256 : len;

    for (i = 0; i < len; i++) {
        if ((i & 0x1f) == 0) {
            sprintf(str, "%04x: ", i);
        }
        sprintf(&str[strlen(str)], "%02x", data[i]);
        if ((i & 0x1f) == 0x1f) {
            sprintf(&str[strlen(str)], "\n");
            printk(str);
            continue;
        }
        if ((i & 0x3) == 0x3) {
            sprintf(&str[strlen(str)], " ");
        }
    }
    if ((i & 0x1f) != 0) {
        sprintf(&str[strlen(str)], "\n");
        printk(str);
    }
    printk("\n");
}

static inline void
_adapter_pkt_rx_mark_skb_vlan_tagged(
    struct sk_buff *skb,
    uint16_t tpid,
    uint16_t tci,
    int rcpu_encap)
{
    if (rcpu_encap) {
        kal_vlan_hwaccel_put_tag(skb, ETH_P_8021Q, tci);
    } else {
        if (tpid == ETH_P_8021AD) {
            kal_vlan_hwaccel_put_tag(skb, ETH_P_8021AD, tci);
        } else {
            kal_vlan_hwaccel_put_tag(skb, ETH_P_8021Q, tci);
        }
    }
}

static void
_adapter_pkt_recv(struct net_device *ndev, struct sk_buff *skb)
{
    struct ngknet_private *priv = netdev_priv(ndev);
    struct ngknet_dev *dev = priv->bkn_dev;
    int kdev = dev->dev_info.dev_no;
    struct adapter_dev *adev = &adapter_devices[kdev];
    ngknet_netif_t *netif = &priv->netif;
    struct pkt_buf *pkb = (struct pkt_buf *)skb->data;
    struct pkt_hdr *pkh = &pkb->pkh;
    int pkt_len = PKT_HDR_SIZE + pkh->meta_len + pkh->data_len;
    /*
     * By default, NGKNET does not support marking the vlan tag
     */
    /*
    uint8_t *pkt = &skb->data[PKT_HDR_SIZE + pkh->meta_len];
    */

    DBG_VERB(("_adapter_pkt_recv: skb->len = %d, pkt_len = %d\n", skb->len, pkt_len));
    /* Indicate the packet is parsed by adapter_rx_parser(). */
    if ((skb->len - pkt_len) == NGKNET_SCRATCH_DATA_BYTES) {
        /* Remove the scratch data which was put to the end of packet */
        skb_trim(skb, pkt_len);
        /* To API rx netif, the meta_len should be zero. */
        if (netif->id == adev->def_netif) {
            pkh->data_len += pkh->meta_len;
            pkh->meta_len  = 0;
        }
    }
    if ((debug & DBG_LVL_PKT)) {
        _adapter_pkt_dump(skb->data, skb->len);
    }
    /*
     * Mark packet as VLAN-tagged, otherwise newer
     * kernels will strip the tag.
     */
    /*
    if (!(pkb->pkh.attrs & PDMA_RX_STRIP_TAG)) {
        uint16_t vlan_proto = PKT_U16_GET(pkt, 12);
        uint16_t tci = PKT_U16_GET(pkt, 14);
        int renc = (netif->flags & NGKNET_NETIF_F_RCPU_ENCAP) ? 1 : 0;
        DBG_VERB(("_adapter_pkt_recv: vlan_proto = 0x%04x, tci = %d, renc = %s\n", vlan_proto, tci, renc ? "true":"false"));

        _adapter_pkt_rx_mark_skb_vlan_tagged(skb, vlan_proto, tci, renc);
    }
    */
    /* See: ngknet_pkt_recv() */
    if (adev->pkt_recv) {
        adev->pkt_recv(ndev, skb);
    }
    DBG_VERB(("\n--> Sent to netif(%d)...\n", netif->id));
    return;
}

static int
_adapter_dev_netif_cb(
    ngknet_dev_info_t *dinfo,
    ngknet_netif_t *netif)
{
    struct adapter_dev *adev = NULL;
    struct net_device *ndev = NULL;
    struct ngknet_private *priv = NULL;
    struct ngknet_dev *dev = NULL;
    int kdev;

    if (!dinfo || !netif) {
        return -1;
    }
    kdev = dinfo->dev_no;
    adev = &adapter_devices[kdev];
    DBG_VERB(("_adapter_dev_netif_cb: Kdev%d, Id=%d.\n", kdev, netif->id));
    if (ADAPTER_IS_ACTIVED(adev)) {
        dev  = adev->dev;
        ndev = dev->vdev[netif->id];
        priv = netdev_priv(ndev);
        priv->pkt_recv = _adapter_pkt_recv;
        DBG_VERB(("_adapter_dev_netif_cb: Pass.\n"));
        return 0;
    }
    return -1;
}

static void
_adapter_dev_isr(
    void *isr_data)
{
    if (isr_data)
    {
        struct adapter_dev *adev = (struct adapter_dev *)isr_data;
        if (ADAPTER_IS_ACTIVED(adev)) {
            if (adev->isr_func) {
                int rv;
                rv = adev->isr_func(adev->isr_data);
                adev->isr_received++;
                if (rv == IRQ_HANDLED) {
                    adev->isr_handled++;
                }
            }
        }
    }
}

static int
_adapter_dev_intr_connect(
    int kdev,
    uint32_t irq_num,
    isr_func_f isr_func,
    void *isr_data)
{
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (ADAPTER_IS_ACTIVED(adev)) {
        adev->irq_num = irq_num;
        adev->isr_func = isr_func;
        adev->isr_data = isr_data;
        /* Register interrupt handler */
        kernel_bde->interrupt_connect(kdev | LKBDE_ISR2_DEV, _adapter_dev_isr, adev);
        adev->isr_connected++;
        return 0;
    }
    return -1;
}

static int
_adapter_dev_intr_disconnect(
    int kdev,
    unsigned int irq_num)
{
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (ADAPTER_IS_ACTIVED(adev)) {
        kernel_bde->interrupt_disconnect(kdev | LKBDE_ISR2_DEV);
        adev->isr_connected--;
        adev->irq_num = 0;
        adev->isr_func = NULL;
        adev->isr_data = NULL;
        return 0;
    }
    return -1;
}


static int
_adapter_dev_init(
    int kdev)
{
    struct ngknet_dev *dev = &ngknet_devices[kdev];
    struct pdma_dev *pdev = &dev->pdma_dev;
    struct net_device *ndev = dev->net_dev;
    struct ngknet_private *priv = netdev_priv(ndev);
    if (pdev->attached) {
        struct dev_ctrl *ctrl = &pdev->ctrl;
        struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
        struct adapter_dev *adev = &adapter_devices[kdev];
        sal_memset(adev, 0, sizeof(*adev));
        adev->pdma_dev = pdev;
        adev->pdma_hw = hw;
        adev->dev = dev;
        if (strcmp(hw->info.name, CMICR_DEV_NAME) == 0) {
            adev->cmic_type = ADAPTER_CMIC_T_CMICR;
            adev->nof_chans = 16; /* CMICR_PDMA_CMC_CHAN */
            adev->irq_fmask = CMICR_TXRX_IRQ_MASK;
            adev->irq_mask_reg = PAXB_0_INTC_SET_INTR_ENABLE_REG5r;
            adev->iio_base = adev->irq_mask_reg & ~0xfff;
        } else if (strcmp(hw->info.name, CMICX_DEV_NAME) == 0) {
            adev->cmic_type = ADAPTER_CMIC_T_CMICX;
            adev->nof_chans = 8; /* CMICX_PDMA_CMC_CHAN */
            adev->irq_fmask = CMICX_TXRX_IRQ_MASK;
            adev->irq_mask_reg = CMICX_IRQ_ENABr;
            adev->iio_base = adev->irq_mask_reg & ~0xfff;
        } else if (strcmp(hw->info.name, CMICD_DEV_NAME) == 0) {
            adev->cmic_type = ADAPTER_CMIC_T_CMICD;
            return -1;
        } else {
            return -1;
        }
        adev->def_netif = 1;
        if (adev->pkt_recv == NULL) {
            adev->pkt_recv = priv->pkt_recv;
        }
        adev->flags |= ADAPTER_ACTIVE;

        /* Re-attach the hooks */
        do {
            int i;
            for (i = 1; i < NUM_VDEV_MAX + 1; i++) {
                ndev = dev->vdev[i];
                if (ndev) {
                    priv = netdev_priv(ndev);
                    if (priv->pkt_recv && priv->pkt_recv != _adapter_pkt_recv) {
                        priv->pkt_recv = _adapter_pkt_recv;
                    }
                }
            }
            if (pdev->pkt_xmit) {
                adev->pkt_xmit = pdev->pkt_xmit;
                pdev->pkt_xmit = ngknet_adapter_frame_xmit;
            }
        } while(0);
        return 0;
    }
    return -1;
}

static int
_adapter_dev_cleanup(
    int kdev)
{
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (ADAPTER_IS_ACTIVED(adev)) {
        sal_memset(adev, 0, sizeof(*adev));
        return 0;
    }
    return -1;
}

static int
_adapter_init(void)
{
    /* Connect to the kernel bde */
    if ((linux_bde_create(NULL, &kernel_bde) < 0) || kernel_bde == NULL) {
        return -1;
    }
    /*
     * Register a callback function to override priv->pkt_recv(),
     * while create a new netif.
     */
    ngknet_netif_create_cb_register(_adapter_dev_netif_cb);
    return 0;
}

int
ngknet_adapter_frame_recv(
    struct pdma_dev *pdev,
    int queue,
    void *buf)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct sk_buff *skb = (struct sk_buff *)buf;
    struct pkt_buf *pkb = (struct pkt_buf *)skb->data;
    struct pkt_hdr *pkh = &pkb->pkh;
    int kdev = dev->dev_info.dev_no;
    struct adapter_dev *adev = &adapter_devices[kdev];
    int rv = -1, chan_id;
    DBG_VERB(("\n<-- Injected from CNET...\n"));
    DBG_VERB(("Adapter Rx packet (%d bytes) from kdev %d queue %d.\n", skb->len, kdev, queue));
    if ((debug & DBG_LVL_PKT)) {
        _adapter_pkt_dump(skb->data, skb->len);
    }
    if (ADAPTER_IS_ACTIVED(adev)) {
        rv = bcmcnet_pdma_dev_queue_to_chan(&dev->pdma_dev, pkh->queue_id,
                                            PDMA_Q_RX, &chan_id);
        if (SHR_FAILURE(rv)) {
            return rv;
        }
        if (!(adev->rsi.enet_channels & (0x1 << chan_id))) {
            if (!(adev->rsi.flags & NGKNET_F_RX_PKT_UNPARSED)) {
                /* parse the system header. */
                rv = ngknet_rx_parser(adev, skb);
                if (SHR_FAILURE(rv)) {
                    return rv;
                }
            }
        }
    }

    DBG_VERB(("Adapter Rx packet got (%d bytes).\n", skb->len));
    if ((debug & DBG_LVL_PKT)) {
        _adapter_pkt_dump(skb->data, PKT_HDR_SIZE);
        _adapter_pkt_dump(skb->data + PKT_HDR_SIZE, pkh->meta_len);
        _adapter_pkt_dump(skb->data + PKT_HDR_SIZE + pkh->meta_len, pkh->data_len);
        _adapter_pkt_dump(skb->data + PKT_HDR_SIZE + pkh->meta_len + pkh->data_len, skb->len - PKT_HDR_SIZE - pkh->meta_len - pkh->data_len);
    }
    return rv;
}

int
ngknet_adapter_frame_xmit(
    struct pdma_dev *pdev,
    int queue,
    void *buf)
{
    struct ngknet_dev *dev = (struct ngknet_dev *)pdev->priv;
    struct sk_buff *skb = (struct sk_buff *)buf;
    struct pkt_buf *pkb = (struct pkt_buf *)skb->data;
    struct pkt_hdr *pkh = &pkb->pkh;
    int kdev = dev->dev_info.dev_no;
    struct adapter_dev *adev = &adapter_devices[kdev];
    int rv = 0;
    DBG_VERB(("\n--> Send to CNET...\n"));
    DBG_VERB(("Adapter Tx packet (%d bytes) to kdev %d queue %d.\n", skb->len, kdev, queue));
    if ((debug & DBG_LVL_PKT)) {
        _adapter_pkt_dump(skb->data, PKT_HDR_SIZE);
        _adapter_pkt_dump(skb->data + PKT_HDR_SIZE, pkh->meta_len);
        _adapter_pkt_dump(skb->data + PKT_HDR_SIZE + pkh->meta_len, pkh->data_len);
    }
    if (adev->pkt_xmit) {
        rv = adev->pkt_xmit(pdev, queue, buf);
    }
    return rv;
}

bool
ngknet_adapter_filter_scratch_data_match(
     int kdev,
     int chan_id,
     struct sk_buff *skb,
     ngknet_filter_t *filt)
{
    struct adapter_dev * adev = &adapter_devices[kdev];
    if (ADAPTER_IS_ACTIVED(adev)) {
        /*
         * return true to skip the scratch data match.
         */
        if (filt->id != adev->scratch[filt->id].filter_id) {
            return true;
        }
        return ngknet_rx_parser_scratch_data_match(adev, skb, filt);
    }
    return false;
}

int
ngknet_adapter_msg(
    struct ngknet_dev *dev,
    int kdev,
    int cmd,
    int target,
    char *data,
    int len)
{
    struct adapter_dev *adev = &adapter_devices[kdev];
    unsigned long flags;
    if (ADAPTER_IS_ACTIVED(adev)) {
        switch(cmd) {
        case NGKNET_MSG_CMD_RX_START_INFO:
            if (len != sizeof(adev->rsi)) {
                printk("Invalid len (%d, expect %lu)\n", len, sizeof(adev->rsi));
                return -1;
            }
            spin_lock_irqsave(&dev->lock, flags);
            sal_memcpy(&adev->rsi, data, len);
            adev->rsi.flags |= NGKNET_F_RX_START_INFO_DELIVERED;
            spin_unlock_irqrestore(&dev->lock, flags);
            break;
        case NGKNET_MSG_CMD_DBG_LVL_INFO:
            if (len != sizeof(debug)) {
                printk("Invalid len (%d, expect %lu)\n", len, sizeof(debug));
                return -1;
            }
            debug = *(int *)data;
            ngknet_rx_parser_debug_set(debug);
            break;
        case NGKNET_MSG_CMD_SCRATCH_INFO:
            if (target < 0 || target >= NUM_FILTER_MAX) {
                printk("Invalid target (%d)\n", target);
                return -1;
            }
            if (len != sizeof(ngknet_msg_scratch_data_t)) {
                printk("Invalid len (%d, expect %lu)\n", len, sizeof(adev->scratch[target]));
                return -1;
            }
            sal_memcpy(&adev->scratch[target], data, len);
            break;
        case NGKNET_MSG_CMD_SCRATCH_INFO_FETCH:
            if (target < 0 || target >= NUM_FILTER_MAX) {
                printk("Invalid target (%d)\n", target);
                return -1;
            }
            if (len != sizeof(ngknet_msg_scratch_data_t)) {
                printk("Invalid len (%d, expect %lu)\n", len, sizeof(adev->scratch[target]));
                return -1;
            }
            sal_memcpy(data, &adev->scratch[target], len);
            break;
        default:
            break;
        }

        return 0;
    }
    printk("Adapter kdev(%d) is not active for now.\n", kdev);
    return -1;
}

int
ngknet_adapter_device_is_sand(
    int kdev)
{
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (ADAPTER_IS_ACTIVED(adev)) {
        struct ngknet_dev *dev = adev->dev;
        uint32 dev_family = dev->dev_info.dev_id & 0xf000;

        /**
         * 0xb000 : 56xxx (XGS)
         * 0xf000 : 78xxx (XGS)
         * 0x8000 : 88xxx (DNX)
         * 0x9000 : 99xxx (DNX)
         */
        if (dev_family == 0x8000 || dev_family == 0x9000) {
            return 1;
        }
    }
    return 0;
}

struct device *
ngbde_kapi_dma_dev_get(
    int kdev)
{
    void *sd = NULL;
#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    sd = lkbde_get_dma_dev(kdev);
#endif
    if (!sd) {
        return NULL;
    }
    return (struct device *)sd;
}

void *
ngbde_kapi_dma_bus_to_virt(
    int kdev,
    dma_addr_t baddr)
{
#if ADAPTER_DMA_ADDR_DEBUG
    struct adapter_dev *adev = &adapter_devices[kdev];
#endif /* ADAPTER_DMA_ADDR_DEBUG */
    sal_paddr_t paddr;
    void *vaddr = NULL;
    if (kernel_bde == NULL) {
        return NULL;
    }
    paddr = (sal_paddr_t)baddr;
    vaddr = kernel_bde->p2l(kdev, paddr);
#if ADAPTER_DMA_ADDR_DEBUG
    if (ADAPTER_IS_ACTIVED(adev)) {
        adev->p2l_baddr = baddr;
        adev->p2l_paddr = paddr;
        adev->p2l_vaddr = vaddr;
        adev->p2l_cnt++;
    }
#endif /* ADAPTER_DMA_ADDR_DEBUG */
    return vaddr;
}

dma_addr_t
ngbde_kapi_dma_virt_to_bus(
    int kdev,
    void *vaddr)
{
#if ADAPTER_DMA_ADDR_DEBUG
    struct adapter_dev *adev = &adapter_devices[kdev];
#endif /* ADAPTER_DMA_ADDR_DEBUG */
    dma_addr_t baddr;
    sal_paddr_t paddr;
    if (kernel_bde == NULL) {
        return (dma_addr_t)NULL;
    }
    paddr = kernel_bde->l2p(kdev, vaddr);
    baddr = (dma_addr_t)paddr;
#if ADAPTER_DMA_ADDR_DEBUG
    if (ADAPTER_IS_ACTIVED(adev)) {
        adev->l2p_baddr = baddr;
        adev->l2p_paddr = paddr;
        adev->l2p_vaddr = vaddr;
        adev->l2p_cnt++;
    }
#endif /* ADAPTER_DMA_ADDR_DEBUG */
    return baddr;
}

void
ngbde_kapi_pio_write32(
    int kdev,
    uint32_t offs,
    uint32_t val)
{
    if (kernel_bde == NULL) {
        return;
    }
    kernel_bde->write(kdev, offs, val);
    return;
}

uint32_t
ngbde_kapi_pio_read32(
    int kdev,
    uint32_t offs)
{
    if (kernel_bde == NULL) {
        return (uint32_t)-1;
    }
    return kernel_bde->read(kdev, offs);
}

void *
ngbde_kapi_pio_membase(
    int kdev)
{
    if (kernel_bde == NULL) {
        if (_adapter_init() < 0) {
            return NULL;
        }
    }
    return lkbde_get_dev_virt(kdev);
}

void
ngbde_kapi_iio_write32(
    int kdev,
    uint32_t offs,
    uint32_t val)
{
    uint32_t iio_base = 0;
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (kernel_bde == NULL) {
        return;
    }
    if (ADAPTER_IS_ACTIVED(adev)) {
        adev->iio_recorder[adev->iio_recorder_cnt % ADAPTER_IIO_RECORDER_SIZE].op = ADAPTER_IIO_OP_WRITE;
        adev->iio_recorder[adev->iio_recorder_cnt % ADAPTER_IIO_RECORDER_SIZE].offs = offs;
        adev->iio_recorder[adev->iio_recorder_cnt % ADAPTER_IIO_RECORDER_SIZE].val = val;
        adev->iio_recorder_cnt++;
        iio_base = adev->iio_base;
        /**
         * See description of lkbde_irq_mask_set() function.
         * for synchronizing hardware access to the IRQ mask register.
         */
        lkbde_irq_mask_set(kdev | LKBDE_ISR2_DEV | LKBDE_IPROC_REG,
                           iio_base + offs, val, adev->irq_fmask);
        return;
    }
    kernel_bde->iproc_write(kdev, iio_base + offs, val);
    return;
}

uint32_t
ngbde_kapi_iio_read32(
    int kdev,
    uint32_t offs)
{
    uint32_t iio_base = 0;
    uint32_t val;
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (kernel_bde == NULL) {
        return (uint32_t)-1;
    }
    if (ADAPTER_IS_ACTIVED(adev)) {
        iio_base = adev->iio_base;
    }
    val = kernel_bde->iproc_read(kdev, iio_base + offs);
    if (ADAPTER_IS_ACTIVED(adev)) {
        adev->iio_recorder[adev->iio_recorder_cnt % ADAPTER_IIO_RECORDER_SIZE].op = ADAPTER_IIO_OP_READ;
        adev->iio_recorder[adev->iio_recorder_cnt % ADAPTER_IIO_RECORDER_SIZE].offs = offs;
        adev->iio_recorder[adev->iio_recorder_cnt % ADAPTER_IIO_RECORDER_SIZE].val = val;
        adev->iio_recorder_cnt++;
    }
    return val;
}

int
ngbde_kapi_intr_connect(
    int kdev,
    unsigned int irq_num,
    int (*isr_func)(void *),
    void *isr_data)
{
    if (kernel_bde == NULL) {
        return -1;
    }
    if (_adapter_dev_init(kdev) < 0) {
        return -1;
    }
    _adapter_dev_intr_connect(kdev, irq_num, isr_func, isr_data);
    return 0;
}

int
ngbde_kapi_intr_disconnect(
    int kdev,
    unsigned int irq_num)
{
    if (kernel_bde == NULL) {
        return -1;
    }
    _adapter_dev_intr_disconnect(kdev, irq_num);
    _adapter_dev_cleanup(kdev);
    return 0;
}

int
ngbde_kapi_intr_mask_write(
    int kdev,
    unsigned int irq_num,
    uint32_t status_reg,
    uint32_t mask_val)
{
    struct adapter_dev *adev = &adapter_devices[kdev];
    if (ADAPTER_IS_ACTIVED(adev)) {
        adev->irq_mask = mask_val;
        adev->irq_status_reg = status_reg;
        adev->irq_mask_cnt++;
        adev->imask_recorder[adev->imask_recorder_cnt % ADAPTER_IMASK_RECORDER_SIZE].inum = irq_num;
        adev->imask_recorder[adev->imask_recorder_cnt % ADAPTER_IMASK_RECORDER_SIZE].mask_val = mask_val;
        adev->imask_recorder_cnt++;
        return lkbde_irq_mask_set(kdev | LKBDE_ISR2_DEV | LKBDE_IPROC_REG,
                                  adev->irq_mask_reg, adev->irq_mask, adev->irq_fmask);
    }
    return -1;
}

int
ngbde_kapi_knet_connect(
    int kdev,
    knet_func_f knet_func,
    void *knet_data)
{
    return 0;
}

int
ngbde_kapi_knet_disconnect(
    int kdev)
{
    return 0;
}
