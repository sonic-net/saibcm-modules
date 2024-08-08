/*
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
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

#ifndef __BCM_GENL_DEV_H__
#define __BCM_GENL_DEV_H__

extern int
bcmgenl_dev_pktmeta_rx_srcport_get(int dev_no, void *pkt_meta,
                                   uint32_t *srcport);

extern int
bcmgenl_dev_pktmeta_rx_dstport_get(int dev_no, void *pkt_meta,
                                   bool *mcast, uint32_t *dstport);

extern int
bcmgenl_dev_pktmeta_rx_reason_get(int dev_no, void *pkt_meta, uint64_t *reason);

extern int
bcmgenl_dev_rx_reason_sample_source_get(int dev_no, uint64_t *val);

extern int
bcmgenl_dev_dcb_info_get(int dev_no, int *dcb_type, int *dcb_size);

extern int
bcmgenl_dev_init(void);

extern int
bcmgenl_dev_cleanup(void);

#endif /* __BCM_GENL_DEV_H__ */
