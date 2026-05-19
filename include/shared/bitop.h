/*
 * $Id: bitop.h,v 1.16 Broadcom SDK $
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
 *
 * Bit Array Operations
 */

#ifndef _SHR_BITOP_H
#define _SHR_BITOP_H

#include <sal/types.h>

/* Base type for declarations */
#define    SHR_BITDCL        uint32
#define    SHR_BITWID        32

/* (internal) Number of SHR_BITDCLs needed to contain _max bits */
#define    _SHR_BITDCLSIZE(_max)    (((_max) + SHR_BITWID - 1) / SHR_BITWID)
#define    SHRi_BITDCLSIZE(_max)    (((_max) + SHR_BITWID - 1) / SHR_BITWID)

/* Size for giving to malloc and memset to handle _max bits */
#define    SHR_BITALLOCSIZE(_max) (_SHR_BITDCLSIZE(_max) * sizeof (SHR_BITDCL))


/* (internal) Number of SHR_BITDCLs needed to contain from start bit to start bit + range */
#define _SHR_BITDCLSIZE_FROM_START_BIT(_start_bit, _range) (_range + _start_bit -1)/SHR_BITWID - _start_bit/SHR_BITWID + 1

/* Size of SHR_BITDCLs needed to contain from start bit to start bit + range.
   Needed when you want to do autosync */
#define SHR_BITALLOCSIZE_FROM_START_BIT(_start_bit, _range) (_SHR_BITDCLSIZE_FROM_START_BIT(_start_bit, _range) * sizeof (SHR_BITDCL))



/* Declare bit array _n of size _max bits */
#define    SHR_BITDCLNAME(_n, _max) SHR_BITDCL    _n[_SHR_BITDCLSIZE(_max)]
/* Declare bit array _n of size _max bits, and clear it */
#define    SHR_BIT_DCL_CLR_NAME(_n, _max) SHR_BITDCL _n[_SHR_BITDCLSIZE(_max)] = {0}

/* (internal) Generic operation macro on bit array _a, with bit _b */
#define    _SHR_BITOP(_a, _b, _op)    \
        (((_a)[(_b) / SHR_BITWID]) _op (1U << ((_b) % SHR_BITWID)))

/* Specific operations */
#define    SHR_BITGET(_a, _b)    _SHR_BITOP(_a, _b, &)
#define    SHR_BITSET(_a, _b)    _SHR_BITOP(_a, _b, |=)
#define    SHR_BITCLR(_a, _b)    _SHR_BITOP(_a, _b, &= ~)
#define    SHR_BITWRITE(_a, _b, _val)    ((_val) ? SHR_BITSET(_a, _b) : SHR_BITCLR(_a, _b))
#define    SHR_BIT_ITER(_a, _max, _b)            \
           for ((_b) = 0; (_b) < (_max); (_b)++) \
               if ((_a)[(_b) / SHR_BITWID] == 0) \
                   (_b) += (SHR_BITWID - 1);     \
               else if (SHR_BITGET((_a), (_b)))

#define SHR_IS_BITSET(_a, _b)   (_SHR_BITOP(_a, _b, &) ? TRUE : FALSE)

#endif    /* !_SHR_BITOP_H */
