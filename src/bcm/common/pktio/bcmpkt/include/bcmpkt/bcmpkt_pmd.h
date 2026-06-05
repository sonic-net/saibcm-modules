/*! \file bcmpkt_pmd.h
 *
 * Common macros and definitions for PMD.
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

#ifndef BCMPKT_PMD_H
#define BCMPKT_PMD_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <sal/sal_types.h>
#endif

/* For application convenience */
#include <bcmpkt/bcmpkt_util.h>
#include <bcmpkt/bcmpkt_rxpmd_fid.h>
#include <bcmpkt/bcmpkt_flexhdr_field.h>
#include <bcmpkt/bcmpkt_lbhdr_field.h>
#include <bcmpkt/bcmpkt_rxpmd_field.h>
#include <bcmpkt/bcmpkt_txpmd_field.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id.h>

/*! Invalid PMD header field ID. */
#define BCMPKT_FID_INVALID -1

/*! Bitmap array size. */
#define BCMPKT_BITMAP_WORD_SIZE  16

/*! PMD header field ID bit array. */
typedef struct bcmpkt_bitmap_s {
    /*! Bit array */
    uint32_t pbits[BCMPKT_BITMAP_WORD_SIZE];
} bcmpkt_bitmap_t;

#endif /* BCMPKT_PMD_H */
