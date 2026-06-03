/*! \file bcmlrd_chip_variant.h
 *
 * \brief BCMLRD variant definitions
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

#ifndef BCMLRD_CHIP_VARIANT_H
#define BCMLRD_CHIP_VARIANT_H

#include <bcmlrd/chip/bcmlrd_variant_defs.h>

#endif /* BCMLRD_CHIP_VARIANT_H */

#ifndef DOXYGEN_IGNORE_AUTOGEN

#ifdef BCMLTD_VARIANT_OVERRIDE
#error "Use BCMLRD_VARIANT_OVERRIDE instead."
#endif
#ifdef BCMLRD_VARIANT_OVERRIDE
#define BCMLTD_VARIANT_OVERRIDE
#endif
#ifdef BCMLRD_VARIANT_ENTRY
#define BCMLTD_VARIANT_ENTRY(...) BCMLRD_VARIANT_ENTRY(__VA_ARGS__)
#include <bcmltd/bcmltd_variant.h>
#ifdef BCMLRD_VARIANT_OVERRIDE
#undef BCMLRD_VARIANT_OVERRIDE
#endif
#undef BCMLRD_VARIANT_ENTRY
#endif /* BCMLRD_VARIANT_ENTRY */

#endif /* DOXYGEN_IGNORE_AUTOGEN */
