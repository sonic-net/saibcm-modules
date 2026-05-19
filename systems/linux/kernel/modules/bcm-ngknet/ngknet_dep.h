/*! \file ngknet_dep.h
 *
 * Macro definitions for NGKNET dependence.
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

#ifndef NGKNET_DEP_H
#define NGKNET_DEP_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <shr/shr_error.h>
#endif
#include <ngknet_linux.h>

/*! Memorry barrier */
#define MEMORY_BARRIER      smp_mb()

/*! CNET log macros */
#define CNET_INFO(unit, fmt, args...)   printk(KERN_INFO fmt, ##args)
#define CNET_ERROR(unit, fmt, args...)  printk(KERN_ERR fmt, ##args)

struct pdma_dev;

/*! Externs for the required functions. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
extern int _bd##_cnet_pdma_attach(struct pdma_dev *dev); \
extern int _bd##_cnet_pdma_detach(struct pdma_dev *dev);
#include <bcmdrd/bcmdrd_devlist.h>

/*! Create enumeration values from list of supported devices. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    NGKNET_DEV_T_##_bd,
/*! Enumeration for all base device types. */
typedef enum {
    NGKNET_DEV_T_NONE = 0,
#include <bcmdrd/bcmdrd_devlist.h>
    NGKNET_DEV_T_COUNT
} ngknet_dev_type_t;

#endif /* NGKNET_DEP_H */

