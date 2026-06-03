/*! \file bcmpkt_hg3.h
 *
 * Common macros and definitions for Higig3 protocol
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

#ifndef BCMPKT_HG3_H
#define BCMPKT_HG3_H

/* Note, ether type set to same value as reset value of R_GSH_ETHERTYPEr(700) */
/*! Ethernet type used for Higig3 header */
#define BCMPKT_HG3_ETHER_TYPE                     0x2BC

/*! Higig3 base header size (bytes). */
#define BCMPKT_HG3_BASE_HEADER_SIZE_BYTES         8
/*! Higig3 base header size (words). */
#define BCMPKT_HG3_BASE_HEADER_SIZE_WORDS         2

/*! Higig3 extension 0 header size (bytes). */
#define BCMPKT_HG3_EXT0_HEADER_SIZE_BYTES         8
/*! Higig3 extension 0 header size (words). */
#define BCMPKT_HG3_EXT0_HEADER_SIZE_WORDS         2

/*! Higig3 header size (bytes). Includes base and ext0 header */
#define BCMPKT_HG3_SIZE_BYTES       (BCMPKT_HG3_BASE_HEADER_SIZE_BYTES + \
                                     BCMPKT_HG3_EXT0_HEADER_SIZE_BYTES)
/*! Higig3 header size (words). Includes base and ext0 header */
#define BCMPKT_HG3_SIZE_WORDS       (BCMPKT_HG3_BASE_HEADER_SIZE_WORDS + \
                                     BCMPKT_HG3_EXT0_HEADER_SIZE_WORDS)

/*! Higig3 extension 0 field max. */
#define BCMPKT_HG3_EXT0_FID_MAX                   32
#endif /* BCMPKT_HG3_H */
