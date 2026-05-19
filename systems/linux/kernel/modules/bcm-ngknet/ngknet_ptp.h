/*! \file ngknet_ptp.h
 *
 * Definitions and APIs declaration for PTP.
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

#ifndef NGKNET_PTP_H
#define NGKNET_PTP_H

#include <linux/skbuff.h>
#include "ngknet_main.h"

/*!
 * \brief PTP Rx config set.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] filter Rx filter.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_config_set(struct net_device *ndev, int *filter);

/*!
 * \brief PTP Tx config set.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] type Tx type.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_config_set(struct net_device *ndev, int type);

/*!
 * \brief PTP Rx HW timestamping get.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] skb Rx packet SKB.
 * \param [out] ts Timestamp value.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_hwts_get(struct net_device *ndev, struct sk_buff *skb, uint64_t *ts);

/*!
 * \brief PTP Tx HW timestamping get.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] skb Tx packet SKB.
 * \param [out] ts Timestamp value.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_hwts_get(struct net_device *ndev, struct sk_buff *skb, uint64_t *ts);

/*!
 * \brief PTP Tx meta set.
 *
 * \param [in] ndev Network device structure point.
 * \param [in] skb Tx packet SKB.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_tx_meta_set(struct net_device *ndev, struct sk_buff *skb);

/*!
 * \brief PTP PHC index get.
 *
 * \param [in] ndev Network device structure point.
 * \param [out] index PHC index.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_phc_index_get(struct net_device *ndev, int *index);

/*!
 * \brief PTP device control.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] cmd Command.
 * \param [in] data Data buffer.
 * \param [in] len Data length.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_dev_ctrl(struct ngknet_dev *dev, int cmd, char *data, int len);

/*!
 * \brief PTP Rx pre-process to get custom header length.
 *
 * If the RX PTP packet is timestamped by the HW and requires
 * timestamp processing then, this function can be used
 * to get the custom/system header length encapsulated by the FW.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] skb Rx packet SKB.
 * \param [out] Custom header length.
 *
 * \retval SHR_E_NONE No errors.
 */
extern int
ngknet_ptp_rx_pre_process(struct net_device *ndev, struct sk_buff *skb, uint32_t *cust_hdr_len);

#endif /* NGKNET_PTP_H */

