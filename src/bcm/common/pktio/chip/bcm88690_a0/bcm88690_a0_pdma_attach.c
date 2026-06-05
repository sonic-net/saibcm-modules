/*! \file bcm88690_a0_pdma_attach.c
 *
 * Initialize PDMA driver resources.
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

#include <bcmcnet/bcmcnet_core.h>
#include <bcmcnet/bcmcnet_dev.h>
#include <bcmcnet/bcmcnet_cmicx.h>

#ifdef PKTIO_KIMPL
#include "ngknet_adapter.h"

static pdma_rx_f ngkent_pkt_recv_func = NULL;

static int
ngknet_dev_pkt_recv(
    struct pdma_dev *dev,
    int queue,
    void *buf)
{
    int rv = 0;
    rv = ngknet_adapter_frame_recv(dev, queue, buf);
    if (rv < 0) {
        return rv;
    }
    if (ngkent_pkt_recv_func) {
        rv = ngkent_pkt_recv_func(dev, queue, buf);
    }
    return rv;
}
#endif /*PKTIO_KIMPL*/

int
bcm88690_a0_cnet_pdma_attach(
    struct pdma_dev *dev)
{
    int rv;
    rv = bcmcnet_cmicx_pdma_driver_attach(dev);
    dev->flags |= PDMA_NO_FCS;
#ifdef PKTIO_KIMPL
    if (ngkent_pkt_recv_func == NULL) {
        ngkent_pkt_recv_func = dev->pkt_recv;
    }
    dev->pkt_recv = ngknet_dev_pkt_recv;
#endif /*PKTIO_KIMPL*/
    return rv;
}

int
bcm88690_a0_cnet_pdma_detach(
    struct pdma_dev *dev)
{
    return bcmcnet_cmicx_pdma_driver_detach(dev);
}
