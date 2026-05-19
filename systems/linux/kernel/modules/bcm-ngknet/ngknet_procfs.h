/*! \file ngknet_procfs.h
 *
 * Procfs-related definitions and APIs for NGKNET module.
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

#ifndef NGKNET_PROCFS_H
#define NGKNET_PROCFS_H

/*!
 * \brief Initialize procfs for KNET driver.
 *
 * Create procfs read/write interfaces.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngknet_procfs_init(void);

/*!
 * \brief Clean up procfs for KNET driver.
 *
 * Clean up resources allocated by \ref ngknet_procfs_init.
 *
 * \return 0 if no errors, otherwise -1.
 */
extern int
ngknet_procfs_cleanup(void);

#endif /* NGKNET_PROCFS_H */

