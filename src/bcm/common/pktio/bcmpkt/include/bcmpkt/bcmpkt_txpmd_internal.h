/*! \file bcmpkt_txpmd_internal.h
 *
 * TX Packet MetaData (TXPMD, called SOBMH in hardware) access interface
 * (Internal use only).
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

#ifndef BCMPKT_TXPMD_INTERNAL_H
#define BCMPKT_TXPMD_INTERNAL_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <shr/shr_types.h>
#endif
#include <bcmpkt/bcmpkt_txpmd_defs.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>

/*!
 * Array of TXPMD field getter functions for a particular device
 * type.
 */
typedef struct bcmpkt_txpmd_fget_s {
    bcmpkt_field_get_f fget[BCMPKT_TXPMD_FID_COUNT];
} bcmpkt_txpmd_fget_t;

/*!
 * Array of TXPMD field setter functions for a particular device
 * type. These functions are used for internally configuring packet
 * filter.
 */
typedef struct bcmpkt_txpmd_fset_s {
    bcmpkt_field_set_f fset[BCMPKT_TXPMD_FID_COUNT];
} bcmpkt_txpmd_fset_t;

/*!
 * Array of TXPMD field address and length getter functions for a multiple
 * words field of a particular device type. *addr is output address and return
 * length.
 */
typedef struct bcmpkt_txpmd_figet_s {
    bcmpkt_ifield_get_f fget[BCMPKT_TXPMD_I_FID_COUNT];
} bcmpkt_txpmd_figet_t;

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern void _bd##_txpmd_view_info_get(bcmpkt_pmd_view_info_t *info);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>

#endif /* BCMPKT_TXPMD_INTERNAL_H */
