/*! \file ngedk_kapi.h
 *
 * NGEDK kernel API.
 *
 * This file is intended for use by other kernel modules relying on the NGEDK.
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

#ifndef NGEDK_KAPI_H
#define NGEDK_KAPI_H

/*!
 * \brief Converts physical address to virtual address.
 *
 * \param [in] paddr physical address.
 *
 * \retval void * Corresponding virtual address.
 */
extern void *
ngedk_dmamem_map_p2v(dma_addr_t paddr);

#endif /* NGEDK_KAPI_H */

