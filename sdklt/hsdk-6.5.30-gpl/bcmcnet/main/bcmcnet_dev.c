/*! \file bcmcnet_dev.c
 *
 * Utility routines for BCMCNET device.
 *
 */
/*
 * Copyright 2018-2024 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#include <bcmcnet/bcmcnet_core.h>
#include <bcmcnet/bcmcnet_dev.h>
#include <bcmcnet/bcmcnet_rxtx.h>
#include <bcmcnet/bcmcnet_buff.h>

/*!
 * Free resource for a Rx queue
 */
static void
bcn_rx_queues_free(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    int gi, qi;

    for (gi = 0; gi < dev->num_groups; gi++) {
        for (qi = 0; qi < dev->grp_queues; qi++) {
            rxq = (struct pdma_rx_queue *)ctrl->grp[gi].rx_queue[qi];
            if (!rxq) {
                continue;
            }
            sal_free(rxq);
            ctrl->grp[gi].rx_queue[qi] = NULL;
            if (dev->mode == DEV_MODE_HNET && ctrl->grp[gi].vnet_rxq[qi]) {
                sal_free(ctrl->grp[gi].vnet_rxq[qi]);
                ctrl->grp[gi].vnet_rxq[qi] = NULL;
            }
        }
    }
}

/*!
 * Allocate resource for a Rx queue
 */
static int
bcn_rx_queues_alloc(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = NULL, *vrxq = NULL;
    int gi, qi;

    for (gi = 0; gi < dev->num_groups; gi++) {
        for (qi = 0; qi < dev->grp_queues; qi++) {
            rxq = sal_alloc(sizeof(*rxq), "bcmcnetRxQueue");
            if (!rxq) {
                goto error;
            }
            ctrl->grp[gi].rx_queue[qi] = rxq;
            sal_memset(rxq, 0, sizeof(*rxq));
            rxq->group_id = gi;
            rxq->chan_id = qi + gi * dev->grp_queues;
            rxq->ctrl = ctrl;
            if (dev->mode == DEV_MODE_HNET) {
                vrxq = sal_alloc(sizeof(*vrxq), "bcmcnetVnetRxQueue");
                if (!vrxq) {
                    goto error;
                }
                sal_memset(vrxq, 0, sizeof(*vrxq));
                vrxq->group_id = gi;
                vrxq->chan_id = qi + gi * dev->grp_queues;
                vrxq->ctrl = ctrl;
                ctrl->grp[gi].vnet_rxq[qi] = vrxq;
            }
        }
    }

    return SHR_E_NONE;

error:
    bcn_rx_queues_free(dev);

    return SHR_E_MEMORY;
}

/*!
 * Free resource for a Tx queue
 */
static void
bcn_tx_queues_free(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = NULL;
    int gi, qi;

    for (gi = 0; gi < dev->num_groups; gi++) {
        for (qi = 0; qi < dev->grp_queues; qi++) {
            txq = (struct pdma_tx_queue *)ctrl->grp[gi].tx_queue[qi];
            if (!txq) {
                continue;
            }
            if (txq->sem) {
                sal_sem_destroy(txq->sem);
            }
            sal_free(txq);
            ctrl->grp[gi].tx_queue[qi] = NULL;
            if (dev->mode == DEV_MODE_HNET && ctrl->grp[gi].vnet_txq[qi]) {
                sal_free(ctrl->grp[gi].vnet_txq[qi]);
                ctrl->grp[gi].vnet_txq[qi] = NULL;
            }
        }
    }
}

/*!
 * Allocate resource for a Tx queue
 */
static int
bcn_tx_queues_alloc(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = NULL, *vtxq = NULL;
    int gi, qi;

    for (gi = 0; gi < dev->num_groups; gi++) {
        for (qi = 0; qi < dev->grp_queues; qi++) {
            txq = sal_alloc(sizeof(*txq), "bcmcnetTxQueue");
            if (!txq) {
                goto error;
            }
            ctrl->grp[gi].tx_queue[qi] = txq;
            sal_memset(txq, 0, sizeof(*txq));
            txq->group_id = gi;
            txq->chan_id = qi + gi * dev->grp_queues;
            txq->ctrl = ctrl;
            txq->sem = sal_sem_create("bcmcnetTxMutexSem", SAL_SEM_BINARY, 1);
            if (!txq->sem) {
                goto error;
            }
            if (dev->mode == DEV_MODE_HNET) {
                vtxq = sal_alloc(sizeof(*vtxq), "bcmcnetVnetTxQueue");
                if (!vtxq) {
                    goto error;
                }
                sal_memset(vtxq, 0, sizeof(*vtxq));
                vtxq->group_id = gi;
                vtxq->chan_id = qi + gi * dev->grp_queues;
                vtxq->ctrl = ctrl;
                ctrl->grp[gi].vnet_txq[qi] = vtxq;
            }
        }
    }

    return SHR_E_NONE;

error:
    bcn_tx_queues_free(dev);

    return SHR_E_MEMORY;
}

/*!
 * \brief Parse Rx groups
 *
 * \param [in] dev Device structure point.
 * \param [in] qbm Rx queue bitmap.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
bcn_rx_queue_group_parse(struct pdma_dev *dev, uint32_t qbm)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)ctrl->buf_mngr;
    struct pdma_rx_queue *rxq = NULL;
    struct intr_handle *hdl = NULL;
    uint32_t mask;
    int gi, qi, qn;

    ctrl->nb_rxq = 0;
    sal_memset(ctrl->rx_queue, 0, sizeof(ctrl->rx_queue));

    /* Figure out available groups and Rx queues */
    for (gi = 0; gi < dev->num_groups; gi++) {
        if (!ctrl->grp[gi].attached) {
            continue;
        }
        qn = 0;
        mask = 0;
        for (qi = 0; qi < dev->grp_queues; qi++) {
            rxq = (struct pdma_rx_queue *)ctrl->grp[gi].rx_queue[qi];
            hdl = &ctrl->grp[gi].intr_hdl[qi];
            if (1 << (qi + gi * dev->grp_queues) & qbm) {
                /* Set the number of descriptors */
                rxq->nb_desc = ctrl->grp[gi].nb_desc[qi];
                if (!rxq->nb_desc) {
                    rxq->nb_desc = ctrl->nb_desc;
                    ctrl->grp[gi].nb_desc[qi] = rxq->nb_desc;
                }
                /* Set Rx buffer size */
                rxq->buf_size = ctrl->grp[gi].rx_size[qi];
                if (rxq->buf_size < RX_BUF_SIZE_MIN) {
                    rxq->buf_size = RX_BUF_SIZE_MIN;
                    ctrl->grp[gi].rx_size[qi] = rxq->buf_size;
                } else if (rxq->buf_size > RX_BUF_SIZE_MAX) {
                    rxq->buf_size = ctrl->rx_buf_size;
                    ctrl->grp[gi].rx_size[qi] = rxq->buf_size;
                }
                rxq->buf_size += dev->rx_ph_size;
                /* Set mode and state for the queue */
                rxq->buf_mode = bm->rx_buf_mode(dev, rxq);
                rxq->state |= PDMA_RX_QUEUE_USED;
                if (dev->flags & PDMA_RX_BATCHING) {
                    rxq->free_thresh = rxq->nb_desc / 4;
                    rxq->state |= PDMA_RX_BATCH_REFILL;
                }
                /* Update queue index */
                rxq->queue_id = ctrl->nb_rxq;
                ctrl->rx_queue[rxq->queue_id] = rxq;
                ctrl->nb_rxq++;
                qn++;
                mask |= 1 << qi;
                /* Set up handler for the queue */
                hdl->queue = rxq->queue_id;
                hdl->dir = PDMA_Q_RX;
                hdl->budget = ctrl->budget < rxq->nb_desc ?
                              ctrl->budget : rxq->nb_desc;
                if (dev->mode == DEV_MODE_HNET) {
                    ctrl->vnet_rxq[rxq->queue_id] = ctrl->grp[gi].vnet_rxq[qi];
                }
            } else {
                rxq->state = 0;
            }
        }

        /* Set group metadata */
        if (qn) {
            ctrl->grp[gi].bm_rxq = mask;
            ctrl->grp[gi].nb_rxq = qn;
        } else {
            ctrl->grp[gi].bm_rxq = 0;
            ctrl->grp[gi].nb_rxq = 0;
        }
    }

    return SHR_E_NONE;
}

/*!
 * \brief Parse Tx groups
 *
 * \param [in] dev Device structure point.
 * \param [in] qbm Tx queue bitmap.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
bcn_tx_queue_group_parse(struct pdma_dev *dev, uint32_t qbm)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_tx_queue *txq = NULL;
    struct intr_handle *hdl = NULL;
    uint32_t mask;
    int gi, qi, qn;

    ctrl->nb_txq = 0;
    sal_memset(ctrl->tx_queue, 0, sizeof(ctrl->tx_queue));

    /* Figure out available groups and Tx queues */
    for (gi = 0; gi < dev->num_groups; gi++) {
        if (!ctrl->grp[gi].attached) {
            continue;
        }
        qn = 0;
        mask = 0;
        for (qi = 0; qi < dev->grp_queues; qi++) {
            txq = (struct pdma_tx_queue *)ctrl->grp[gi].tx_queue[qi];
            hdl = &ctrl->grp[gi].intr_hdl[qi];
            if (1 << (qi + gi * dev->grp_queues) & qbm) {
                /* Set the number of descriptors */
                txq->nb_desc = ctrl->grp[gi].nb_desc[qi];
                if (!txq->nb_desc) {
                    txq->nb_desc = ctrl->nb_desc;
                    ctrl->grp[gi].nb_desc[qi] = txq->nb_desc;
                }
                /* Set mode and state for the queue */
                txq->state |= PDMA_TX_QUEUE_USED;
                if (dev->flags & PDMA_TX_POLLING) {
                    txq->free_thresh = txq->nb_desc / 4;
                    txq->state |= PDMA_TX_QUEUE_POLL;
                }
                /* Update queue index */
                txq->queue_id = ctrl->nb_txq;
                ctrl->tx_queue[txq->queue_id] = txq;
                ctrl->nb_txq++;
                qn++;
                mask |= 1 << qi;
                /* Set up handler for the queue */
                hdl->queue = txq->queue_id;
                hdl->dir = PDMA_Q_TX;
                hdl->budget = ctrl->budget < txq->nb_desc ?
                              ctrl->budget : txq->nb_desc;
                if (dev->mode == DEV_MODE_HNET) {
                    ctrl->vnet_txq[txq->queue_id] = ctrl->grp[gi].vnet_txq[qi];
                }
            } else {
                txq->state = 0;
            }
        }

        /* Set group metadata */
        if (qn) {
            ctrl->grp[gi].bm_txq = mask;
            ctrl->grp[gi].nb_txq = qn;
        } else {
            ctrl->grp[gi].bm_txq = 0;
            ctrl->grp[gi].nb_txq = 0;
        }
    }

    return SHR_E_NONE;
}

/*!
 * \brief Configure device
 *
 * \param [in] dev Device structure point.
 * \param [in] bm_rxq Rx queue bitmap.
 * \param [in] bm_txq Tx queue bitmap.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
bcmcnet_pdma_config(struct pdma_dev *dev, uint32_t bm_rxq, uint32_t bm_txq)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    int gi;

    if (!bm_rxq || !bm_txq || (bm_rxq & bm_txq)) {
        return SHR_E_PARAM;
    }

    bcn_rx_queue_group_parse(dev, bm_rxq);
    bcn_tx_queue_group_parse(dev, bm_txq);

    for (gi = 0; gi < dev->num_groups; gi++) {
        if (!ctrl->grp[gi].attached) {
            continue;
        }
        /* Update group metadata */
        if (!ctrl->grp[gi].bm_rxq && !ctrl->grp[gi].bm_txq) {
            ctrl->grp[gi].attached = false;
            ctrl->bm_grp &= ~(1 << gi);
            ctrl->nb_grp--;
            continue;
        }
        ctrl->grp[gi].ctrl = ctrl;
        ctrl->grp[gi].id = gi;
        ctrl->grp[gi].irq_mask = 0;
    }

    return hw->hdls.hw_config(hw);
}

/*!
 * Close device
 */
static int
bcmcnet_pdma_close(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)dev->ctrl.hw;
    int gi;

    hw->hdls.hw_reset(hw);

    for (gi = 0; gi < dev->num_groups; gi++) {
        if (!ctrl->grp[gi].attached) {
            continue;
        }
        /* Reset group metadata */
        ctrl->bm_grp &= ~(1 << gi);
        ctrl->nb_grp--;
        ctrl->grp[gi].irq_mask = 0;
        ctrl->grp[gi].poll_queues = 0;
        ctrl->grp[gi].attached = false;
    }

    bcn_rx_queues_free(dev);
    bcn_tx_queues_free(dev);

    return SHR_E_NONE;
}

/*!
 * Suspend device
 */
static int
bcmcnet_pdma_suspend(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        bcmcnet_pdma_rx_queue_suspend(dev, qi);
    }

    if (dev->ndev_detach) {
        dev->ndev_detach(dev);
    } else {
        for (qi = 0; qi < ctrl->nb_txq; qi++) {
            bcmcnet_pdma_tx_queue_suspend(dev, qi);
        }
    }

    return SHR_E_NONE;
}

/*!
 * Resume device
 */
static int
bcmcnet_pdma_resume(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    uint32_t qi;

    if (dev->ndev_attach) {
        dev->ndev_attach(dev);
    } else {
        for (qi = 0; qi < ctrl->nb_txq; qi++) {
            bcmcnet_pdma_tx_queue_resume(dev, qi);
        }
    }

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        bcmcnet_pdma_rx_queue_resume(dev, qi);
    }

    return SHR_E_NONE;
}

/*!
 * Get device information
 */
static void
bcmcnet_pdma_info_get(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;
    uint32_t qi;

    sal_strncpy(dev->info.dev_name, dev->name, sizeof(dev->info.dev_name) - 1);
    dev->info.dev_name[sizeof(dev->info.dev_name) - 1] = 0;
    dev->info.dev_id = dev->dev_id;
    dev->info.dev_type = dev->dev_type;
    dev->info.max_groups = hw->info.num_cmcs;
    dev->info.max_queues = hw->info.num_chans;
    dev->info.bm_groups = ctrl->bm_grp;
    dev->info.bm_rx_queues = ctrl->bm_rxq;
    dev->info.bm_tx_queues = ctrl->bm_txq;
    dev->info.nb_groups = ctrl->nb_grp;
    dev->info.nb_rx_queues = ctrl->nb_rxq;
    dev->info.nb_tx_queues = ctrl->nb_txq;
    dev->info.rx_desc_size = hw->info.rx_dcb_size;
    dev->info.tx_desc_size = hw->info.tx_dcb_size;
    dev->info.rx_ph_size = hw->info.rx_ph_size;
    dev->info.tx_ph_size = hw->info.tx_ph_size;
    dev->info.rx_buf_dflt = ctrl->rx_buf_size;
    dev->info.nb_desc_dflt = ctrl->nb_desc;

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rxq = (struct pdma_rx_queue *)ctrl->rx_queue[qi];
        if (!rxq) {
            continue;
        }
        dev->info.rx_buf_size[qi] = rxq->buf_size;
        dev->info.nb_rx_desc[qi] = rxq->nb_desc;
        dev->info.rxq_state[qi] = rxq->state;
    }

    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        txq = (struct pdma_tx_queue *)ctrl->tx_queue[qi];
        if (!txq) {
            continue;
        }
        dev->info.nb_tx_desc[qi] = txq->nb_desc;
        dev->info.txq_state[qi] = txq->state;
    }
}

/*!
 * Get device statistics
 */
static void
bcmcnet_pdma_stats_get(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;
    uint32_t packets = 0, bytes = 0, dropped = 0, errors = 0, nomems = 0, xoffs = 0;
    uint32_t head_errors = 0, data_errors = 0, cell_errors = 0;
    uint32_t qi;

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rxq = (struct pdma_rx_queue *)ctrl->rx_queue[qi];
        if (!rxq) {
            continue;
        }
        packets += rxq->stats.packets;
        bytes += rxq->stats.bytes;
        dropped += rxq->stats.dropped;
        errors += rxq->stats.errors;
        head_errors += rxq->stats.head_errors;
        data_errors += rxq->stats.data_errors;
        cell_errors += rxq->stats.cell_errors;
        nomems += rxq->stats.nomems;
        dev->stats.rxq_packets[qi] = rxq->stats.packets;
        dev->stats.rxq_bytes[qi] = rxq->stats.bytes;
        dev->stats.rxq_dropped[qi] = rxq->stats.dropped;
        dev->stats.rxq_errors[qi] = rxq->stats.errors;
        dev->stats.rxq_head_errors[qi] = rxq->stats.head_errors;
        dev->stats.rxq_data_errors[qi] = rxq->stats.data_errors;
        dev->stats.rxq_cell_errors[qi] = rxq->stats.cell_errors;
        dev->stats.rxq_nomems[qi] = rxq->stats.nomems;
    }

    dev->stats.rx_packets = packets;
    dev->stats.rx_bytes = bytes;
    dev->stats.rx_dropped = dropped;
    dev->stats.rx_errors = errors;
    dev->stats.rx_head_errors = head_errors;
    dev->stats.rx_data_errors = data_errors;
    dev->stats.rx_cell_errors = cell_errors;
    dev->stats.rx_nomems = nomems;

    packets = bytes = dropped = errors = 0;
    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        txq = (struct pdma_tx_queue *)ctrl->tx_queue[qi];
        if (!txq) {
            continue;
        }
        packets += txq->stats.packets;
        bytes += txq->stats.bytes;
        dropped += txq->stats.dropped;
        errors += txq->stats.errors;
        xoffs += txq->stats.xoffs;
        dev->stats.txq_packets[qi] = txq->stats.packets;
        dev->stats.txq_bytes[qi] = txq->stats.bytes;
        dev->stats.txq_dropped[qi] = txq->stats.dropped;
        dev->stats.txq_errors[qi] = txq->stats.errors;
        dev->stats.txq_xoffs[qi] = txq->stats.xoffs;
    }

    dev->stats.tx_packets = packets;
    dev->stats.tx_bytes = bytes;
    dev->stats.tx_dropped = dropped;
    dev->stats.tx_errors = errors;
    dev->stats.tx_xoffs = xoffs;
}

/*!
 * Reset device statistics
 */
static void
bcmcnet_pdma_stats_reset(struct pdma_dev *dev)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;
    uint32_t qi;

    sal_memset(&dev->stats, 0, sizeof(struct bcmcnet_dev_stats));

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rxq = (struct pdma_rx_queue *)ctrl->rx_queue[qi];
        if (!rxq) {
            continue;
        }
        rxq->stats.packets = 0;
        rxq->stats.bytes = 0;
        rxq->stats.dropped = 0;
        rxq->stats.errors = 0;
        rxq->stats.head_errors = 0;
        rxq->stats.data_errors = 0;
        rxq->stats.cell_errors = 0;
        rxq->stats.nomems = 0;
    }

    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        txq = (struct pdma_tx_queue *)ctrl->tx_queue[qi];
        if (!txq) {
            continue;
        }
        txq->stats.packets = 0;
        txq->stats.bytes = 0;
        txq->stats.dropped = 0;
        txq->stats.errors = 0;
        txq->stats.xoffs = 0;
    }
}

/*!
 * Convert logic queue to physical queue
 */
static int
bcmcnet_pdma_lq_to_pq(struct pdma_dev *dev, int queue, int dir, int *chan)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    if (dir == PDMA_Q_RX) {
        rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
        if (rxq->state & PDMA_RX_QUEUE_USED) {
            *chan = rxq->chan_id;
            return SHR_E_NONE;
        }
    } else {
        txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
        if (txq->state & PDMA_TX_QUEUE_USED) {
            *chan = txq->chan_id;
            return SHR_E_NONE;
        }
    }

    return SHR_E_UNAVAIL;
}

/*!
 * Convert physical queue to logic queue
 */
static int
bcmcnet_pdma_pq_to_lq(struct pdma_dev *dev, int chan, int *queue, int *dir)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;

    rxq = ctrl->grp[chan / dev->grp_queues].rx_queue[chan % dev->grp_queues];
    if (rxq->state & PDMA_RX_QUEUE_USED) {
        *queue = rxq->queue_id;
        *dir = PDMA_Q_RX;
        return SHR_E_NONE;
    }

    txq = ctrl->grp[chan / dev->grp_queues].tx_queue[chan % dev->grp_queues];
    if (txq->state & PDMA_TX_QUEUE_USED) {
        *queue = txq->queue_id;
        *dir = PDMA_Q_TX;
        return SHR_E_NONE;
    }

    return SHR_E_UNAVAIL;
}

/*!
 * Start Rx queue
 */
static int
bcmcnet_pdma_rx_queue_start(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    rxq->state |= PDMA_RX_QUEUE_ACTIVE;

    return hw->hdls.chan_start(hw, rxq->chan_id);
}

/*!
 * Stop Rx queue
 */
static int
bcmcnet_pdma_rx_queue_stop(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    rxq->state &= ~PDMA_RX_QUEUE_ACTIVE;

    return hw->hdls.chan_stop(hw, rxq->chan_id);
}

/*!
 * Start Tx queue
 */
static int
bcmcnet_pdma_tx_queue_start(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    txq->state |= PDMA_TX_QUEUE_ACTIVE;

    return dev->flags & PDMA_CHAIN_MODE ? SHR_E_NONE :
           hw->hdls.chan_start(hw, txq->chan_id);
}

/*!
 * Stop Tx queue
 */
static int
bcmcnet_pdma_tx_queue_stop(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    txq->state &= ~PDMA_TX_QUEUE_ACTIVE;

    return hw->hdls.chan_stop(hw, txq->chan_id);
}

/*!
 * Enable Rx queue interrupt
 */
static int
bcmcnet_pdma_rx_queue_intr_enable(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];

    return hw->hdls.chan_intr_enable(hw, rxq->chan_id);
}

/*!
 * Disable Rx queue interrupt
 */
static int
bcmcnet_pdma_rx_queue_intr_disable(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];

    return hw->hdls.chan_intr_disable(hw, rxq->chan_id);
}

/*!
 * Acknowledge Rx queue interrupt
 */
static int
bcmcnet_pdma_rx_queue_intr_ack(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];

    return hw->hdls.chan_clear(hw, rxq->chan_id);
}

/*!
 * Query Rx queue interrupt
 */
static int
bcmcnet_pdma_rx_queue_intr_query(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];

    return hw->hdls.chan_intr_query(hw, rxq->chan_id);
}

/*!
 * Check Rx queue interrupt
 */
static int
bcmcnet_pdma_rx_queue_intr_check(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];

    return hw->hdls.chan_intr_check(hw, rxq->chan_id);
}

/*!
 * Enable Tx queue interrupt
 */
static int
bcmcnet_pdma_tx_queue_intr_enable(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];

    if (txq->state & PDMA_TX_QUEUE_POLL) {
        return SHR_E_NONE;
    } else {
        return hw->hdls.chan_intr_enable(hw, txq->chan_id);
    }
}

/*!
 * Disable Tx queue interrupt
 */
static int
bcmcnet_pdma_tx_queue_intr_disable(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];

    if (txq->state & PDMA_TX_QUEUE_POLL) {
        return SHR_E_NONE;
    } else {
        return hw->hdls.chan_intr_disable(hw, txq->chan_id);
    }
}

/*!
 * Acknowledge Tx queue interrupt
 */
static int
bcmcnet_pdma_tx_queue_intr_ack(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];

    return hw->hdls.chan_clear(hw, txq->chan_id);
}

/*!
 * Query Tx queue interrupt
 */
static int
bcmcnet_pdma_tx_queue_intr_query(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];

    return hw->hdls.chan_intr_query(hw, txq->chan_id);
}

/*!
 * Check Tx queue interrupt
 */
static int
bcmcnet_pdma_tx_queue_intr_check(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];

    return hw->hdls.chan_intr_check(hw, txq->chan_id);
}

/*!
 * \brief Device operation functions.
 */
static const struct dev_ops pdma_dev_ops = {
    .dev_config                 = bcmcnet_pdma_config,
    .dev_close                  = bcmcnet_pdma_close,
    .dev_suspend                = bcmcnet_pdma_suspend,
    .dev_resume                 = bcmcnet_pdma_resume,
    .dev_info_get               = bcmcnet_pdma_info_get,
    .dev_stats_get              = bcmcnet_pdma_stats_get,
    .dev_stats_reset            = bcmcnet_pdma_stats_reset,
    .dev_lq_to_pq               = bcmcnet_pdma_lq_to_pq,
    .dev_pq_to_lq               = bcmcnet_pdma_pq_to_lq,
    .rx_queue_start             = bcmcnet_pdma_rx_queue_start,
    .rx_queue_stop              = bcmcnet_pdma_rx_queue_stop,
    .tx_queue_start             = bcmcnet_pdma_tx_queue_start,
    .tx_queue_stop              = bcmcnet_pdma_tx_queue_stop,
    .rx_queue_setup             = bcmcnet_pdma_rx_queue_setup,
    .rx_queue_release           = bcmcnet_pdma_rx_queue_release,
    .rx_queue_restore           = bcmcnet_pdma_rx_queue_restore,
    .rx_vqueue_setup            = bcmcnet_pdma_rx_vqueue_setup,
    .rx_vqueue_release          = bcmcnet_pdma_rx_vqueue_release,
    .tx_queue_setup             = bcmcnet_pdma_tx_queue_setup,
    .tx_queue_release           = bcmcnet_pdma_tx_queue_release,
    .tx_queue_restore           = bcmcnet_pdma_tx_queue_restore,
    .tx_vqueue_setup            = bcmcnet_pdma_tx_vqueue_setup,
    .tx_vqueue_release          = bcmcnet_pdma_tx_vqueue_release,
    .rx_queue_intr_enable       = bcmcnet_pdma_rx_queue_intr_enable,
    .rx_queue_intr_disable      = bcmcnet_pdma_rx_queue_intr_disable,
    .rx_queue_intr_ack          = bcmcnet_pdma_rx_queue_intr_ack,
    .rx_queue_intr_query        = bcmcnet_pdma_rx_queue_intr_query,
    .rx_queue_intr_check        = bcmcnet_pdma_rx_queue_intr_check,
    .tx_queue_intr_enable       = bcmcnet_pdma_tx_queue_intr_enable,
    .tx_queue_intr_disable      = bcmcnet_pdma_tx_queue_intr_disable,
    .tx_queue_intr_ack          = bcmcnet_pdma_tx_queue_intr_ack,
    .tx_queue_intr_query        = bcmcnet_pdma_tx_queue_intr_query,
    .tx_queue_intr_check        = bcmcnet_pdma_tx_queue_intr_check,
    .rx_queue_suspend           = bcmcnet_pdma_rx_queue_suspend,
    .rx_queue_resume            = bcmcnet_pdma_rx_queue_resume,
    .tx_queue_wakeup            = bcmcnet_pdma_tx_queue_wakeup,
    .rx_queue_poll              = bcmcnet_pdma_rx_queue_poll,
    .tx_queue_poll              = bcmcnet_pdma_tx_queue_poll,
    .group_poll                 = bcmcnet_pdma_group_poll,
};

/*!
 * Open a device
 */
int
bcmcnet_pdma_open(struct pdma_dev *dev)
{
    struct pdma_hw *hw = (struct pdma_hw *)dev->ctrl.hw;
    struct intr_handle *hdl = NULL;
    int chan, gi, qi;

    if (!hw) {
        return SHR_E_INIT;
    }

    /* Initialize the hardware */
    hw->hdls.hw_reset(hw);
    hw->hdls.hw_init(hw);

    if ((uint32_t)dev->num_groups > hw->info.num_cmcs) {
        return SHR_E_PARAM;
    }
    dev->grp_queues = hw->info.cmc_chans;
    dev->num_queues = hw->info.num_chans;
    dev->rx_ph_size = hw->info.rx_ph_size;
    dev->tx_ph_size = hw->info.tx_ph_size;
    dev->ctrl.nb_desc = NUM_RING_DESC;
    dev->ctrl.budget = NUM_RXTX_BUDGET;
    dev->ctrl.rx_desc_size = hw->info.rx_dcb_size;
    dev->ctrl.tx_desc_size = hw->info.tx_dcb_size;

    /* Initialize interrupt handler */
    for (chan = 0; chan < dev->num_queues; chan++) {
        gi = chan / dev->grp_queues;
        qi = chan % dev->grp_queues;
        hdl = &dev->ctrl.grp[gi].intr_hdl[qi];
        hdl->unit = dev->unit;
        hdl->group = gi;
        hdl->chan = chan;
        hdl->dev = dev;
        hdl->inum = hw->hdls.chan_intr_num_get(hw, chan);
        if (hdl->inum < 0) {
            return SHR_E_INTERNAL;
        }
    }

    /* Initialize buffer manager */
    bcmcnet_buf_mngr_init(dev);

    /* Allocate all the queues */
    bcn_rx_queues_alloc(dev);
    bcn_tx_queues_alloc(dev);

    dev->pkt_xmit = bcmcnet_pdma_tx_queue_xmit;

    dev->ops = (struct dev_ops *)&pdma_dev_ops;

    return SHR_E_NONE;
}

/*!
 * Coalesce Rx interrupt
 */
int
bcmcnet_pdma_rx_queue_int_coalesce(struct pdma_dev *dev, int queue, int count, int timer)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX ||
        (uint32_t)queue >= ctrl->nb_rxq) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];
    rxq->intr_coalescing = 1;
    rxq->ic_val = (count & 0x7fff) << 16 | (timer & 0xffff);

    return hw->hdls.chan_intr_coalesce(hw, rxq->chan_id, count, timer);
}

/*!
 * Coalesce Tx interrupt
 */
int
bcmcnet_pdma_tx_queue_int_coalesce(struct pdma_dev *dev, int queue, int count, int timer)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX ||
        (uint32_t)queue >= ctrl->nb_txq) {
        return SHR_E_PARAM;
    }

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];
    txq->intr_coalescing = 1;
    txq->ic_val = (count & 0x7fff) << 16 | (timer & 0xffff);

    return hw->hdls.chan_intr_coalesce(hw, txq->chan_id, count, timer);
}

/*!
 * Dump Rx queue registers
 */
int
bcmcnet_pdma_rx_queue_reg_dump(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_rx_queue *rxq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX ||
        (uint32_t)queue >= ctrl->nb_rxq) {
        return SHR_E_PARAM;
    }

    rxq = (struct pdma_rx_queue *)ctrl->rx_queue[queue];

    return hw->hdls.chan_reg_dump(hw, rxq->chan_id);
}

/*!
 * Dump Tx queue registers
 */
int
bcmcnet_pdma_tx_queue_reg_dump(struct pdma_dev *dev, int queue)
{
    struct dev_ctrl *ctrl = &dev->ctrl;
    struct pdma_hw *hw = (struct pdma_hw *)ctrl->hw;
    struct pdma_tx_queue *txq = NULL;

    if ((uint32_t)queue >= NUM_Q_MAX ||
        (uint32_t)queue >= ctrl->nb_txq) {
        return SHR_E_PARAM;
    }

    txq = (struct pdma_tx_queue *)ctrl->tx_queue[queue];

    return hw->hdls.chan_reg_dump(hw, txq->chan_id);
}

