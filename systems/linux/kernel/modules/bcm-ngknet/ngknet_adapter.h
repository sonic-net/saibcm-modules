/*! \file ngknet_adapter.h
 *
 * Definitions and APIs declaration for Adapter.
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

#ifndef NGKNET_ADAPTER_H
#define NGKNET_ADAPTER_H

#include <linux-bde.h>

#include <linux/skbuff.h>
#include <linux/if_vlan.h>

#include <lkm/ngbde_kapi.h>
#include <lkm/ngknet_ioctl.h>
#include <lkm/ngknet_msg.h>

#include "ngknet_main.h"

typedef int (*isr_func_f)(void *);
typedef enum adapter_dev_cmic_type_e {
    ADAPTER_CMIC_T_NONE,
    ADAPTER_CMIC_T_CMICD,
    ADAPTER_CMIC_T_CMICX,
    ADAPTER_CMIC_T_CMICR,
    ADAPTER_CMIC_T_COUNT
} adapter_dev_cmic_type_t;

#define ADAPTER_CMIC_TYPE_STR  { \
    "none", \
    "cmicd", \
    "cmicx", \
    "cmicr", \
}

/*
 * TO debug address mapping.
 */
#ifndef ADAPTER_DMA_ADDR_DEBUG
#define ADAPTER_DMA_ADDR_DEBUG        0
#endif

#define ADAPTER_IIO_OP_READ  1
#define ADAPTER_IIO_OP_WRITE 2

#define ADAPTER_IIO_RECORDER_SIZE    (20)
#define ADAPTER_IMASK_RECORDER_SIZE  (20)


struct adapter_iio_recorder {
    uint32_t op;
    uint32_t offs;
    uint32_t val;
};

struct adapter_imask_recorder {
    int inum;
    uint32_t mask_val;
};

/*!
 * Adapter description
 */
struct adapter_dev {
    /*! Flags */
    int flags;
    /*! Adapter is active */
#define ADAPTER_ACTIVE      (1 << 0)

    /*! PDMA device */
    struct pdma_dev *pdma_dev;

    /*! PDMA hardware */
    struct pdma_hw *pdma_hw;

    /*! PDMA adapter Tx interface */
    pdma_tx_f pkt_xmit;

    /*! NGKNET devices */
    struct ngknet_dev *dev;

    /*! CMIC type, See adapter_dev_cmic_type_t */
    adapter_dev_cmic_type_t cmic_type;

    /*! Number of channels */
    uint32_t nof_chans;

    /*! Interrupt mask */
    uint32_t irq_mask;

    /*! Interrupt mask register */
    uint32_t irq_mask_reg;

    /*! Interrupt filter mask */
    uint32_t irq_fmask;

    /*! Interrupt status register */
    uint32_t irq_status_reg;

    /*! Interrupt mask counter */
    uint32_t irq_mask_cnt;

    /*! Interrupt counter */
    uint32_t irq_num;

    /*! Interrupt SubRoutine counter */
    uint32_t isr_connected;

    /*! Interrupt SubRoutine */
    isr_func_f isr_func;

    /*! Interrupt SubRoutine data */
    void * isr_data;

    /*! Interrupt SubRoutine counter */
    uint32_t isr_received;

    /*! Interrupt handled counter */
    uint32_t isr_handled;
#if ADAPTER_DMA_ADDR_DEBUG
    /*! p2l counter */
    uint32_t p2l_cnt;

    /*! last bus address */
    dma_addr_t p2l_baddr;

    /*! last phy address */
    sal_paddr_t p2l_paddr;

    /*! last virt address */
    void * p2l_vaddr;

    /*! l2p counter */
    uint32_t l2p_cnt;

    /*! last bus address */
    dma_addr_t l2p_baddr;

    /*! last phy address */
    sal_paddr_t l2p_paddr;

    /*! last virt address */
    void * l2p_vaddr;
#endif /* ADAPTER_DMA_ADDR_DEBUG */
    /*! iio_base */
    uint32_t iio_base;

    /*! iio_recorder */
    struct adapter_iio_recorder iio_recorder[ADAPTER_IIO_RECORDER_SIZE];

    /*! iio_recorder counter */
    int iio_recorder_cnt;

    /*! imask_recorder */
    struct adapter_imask_recorder imask_recorder[ADAPTER_IMASK_RECORDER_SIZE];

    /*! imask_recorder counter */
    int imask_recorder_cnt;

    /* Default netif for API tx/rx */
    int def_netif;

    /*! Record the packet receiver that ngknet_private struct used. */
    ngknet_pkt_recv_f pkt_recv;

    /*! Rx start info */
    ngknet_msg_rx_start_info_t rsi;

    /*! Rx scratch data for each filter */
    ngknet_msg_scratch_data_t scratch[NUM_FILTER_MAX];

};

#define ADAPTER_IS_ACTIVED(_adev)                 ((_adev)->flags & ADAPTER_ACTIVE)

#define ADAPTER_IS_CMICR(_adev)                   ((_adev)->cmic_type == ADAPTER_CMIC_T_CMICR)
#define ADAPTER_IS_CMICX(_adev)                   ((_adev)->cmic_type == ADAPTER_CMIC_T_CMICX)
#define ADAPTER_IS_AI(_adev)                      ((_adev)->rsi.flags & NGKNET_F_RX_PKT_AI_FORMAT)

#define device_is_sand(_kdev)                     (ngknet_adapter_device_is_sand(_kdev))


#if LINUX_VERSION_CODE < KERNEL_VERSION(6,5,0)
#define netdev_get_by_name(net, dev, tracker, gfp) \
        dev_get_by_name(net, dev)
#endif /* KERNEL_VERSION(6,5,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
#define netdev_put(dev, tracker) \
        dev_put(dev)
#endif /* KERNEL_VERSION(6,0,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#define dev_net_set(dev, net)
#endif


extern struct proc_ops ngknet_adapter_info_fops;

/*!
 * \brief Adapter frame receiver.
 *
 * \param [in] dev Pkt dma device.
 * \param [in] queue Rx logical queue.
 * \param [in] buf SKB Data buffer.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int ngknet_adapter_frame_recv(
    struct pdma_dev *pdev,
    int queue,
    void *buf);

/*!
 * \brief Adapter frame transmitter.
 *
 * \param [in] dev Pkt dma device.
 * \param [in] queue Rx logical queue.
 * \param [in] buf SKB Data buffer.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int ngknet_adapter_frame_xmit(
    struct pdma_dev *pdev,
    int queue,
    void *buf);

/*!
 * \brief match the scratch data.
 *
 * \param [in] kdev Kernel device number.
 * \param [in] chan_id Dma channel id.
 * \param [in] skb SKB Data buffer.
 * \param [in] filt Packet filter.
 *
 * \retval SHR_E_NONE No errors.
 */
extern bool ngknet_adapter_filter_scratch_data_match(
    int kdev,
    int chan_id,
    struct sk_buff *skb,
    ngknet_filter_t *filt);

/*!
 * \brief Adapter messages.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] kdev NGKNET device unit number.
 * \param [in] cmd Command.
 * \param [in] target Target.
 * \param [in] data Data buffer.
 * \param [in] len Data length.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int ngknet_adapter_msg(
    struct ngknet_dev *dev,
    int kdev,
    int cmd,
    int target,
    char *data,
    int len);

/*!
 * \brief device is sand/DNX.
 *
 * \param [in] kdev NGKNET device number.
 *
 * \retval 0-NOT, 1-YES.
 */
extern int ngknet_adapter_device_is_sand(
    int kdev);

#endif /* NGKNET_ADAPTER_H */

