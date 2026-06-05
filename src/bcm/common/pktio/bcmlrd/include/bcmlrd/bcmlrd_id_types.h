/*! \file bcmlrd_id_types.h
 *
 * \brief Logical Table ID Types
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

#ifndef BCMLRD_ID_TYPES_H
#define BCMLRD_ID_TYPES_H

#include <bcmltd/bcmltd_id_types.h>

/*!
 * \brief Table identifier.
 *
 * Table identifier similar to those used by the DRD.
 *
 */
typedef bcmltd_sid_t bcmlrd_sid_t;   /* Generic table ID local to symbol. */

/*!
 * \brief Field identifier.
 *
 * Field identifiers similar to those used by the DRD.
 *
 */
typedef bcmltd_fid_t bcmlrd_fid_t;   /* DRD compatible field ID local to logical. */

#endif /* BCMLRD_ID_TYPES_H */
