/*! \file bcmdrd_internal.h
 *
 * Internal DRD APIs.
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

#ifndef BCMDRD_INTERNAL_H
#define BCMDRD_INTERNAL_H

#include <bcmdrd/bcmdrd_dev.h>
#ifndef PKTIO_IMPL
#include <bcmdrd/bcmdrd_port.h>

#include <shr/shr_cht.h>
#endif
/*! Pseudo-block-type for device pipes in \ref bcmdrd_pipe_data_t. */
#define BLKTYPE_DEV     0

/*!
 * Cached information about valid pipes.
 *
 * Information is stored per block type and block type 0 is used for
 * the device pipes.
 */
typedef struct bcmdrd_pipe_data_s {

    /*! Map of valid pipes for a given block type. */
    bcmdrd_pipemap_t pipemap;

    /*! Number valid pipes in a given block type. */
    uint16_t num_pipes;

    /*! Maximum valid pipe index for a given block type. */
    uint16_t pipe_max;

} bcmdrd_pipe_data_t;

/*!
 * \brief BCMDRD Device structure.
 */
typedef struct bcmdrd_dev_s {

    /*! Device identification (typically PCI vendor/device ID). */
    bcmdrd_dev_id_t id;

    /*! Global chip flags. */
    uint32_t flags;

    /*! Revision string (e.g. "A0"). */
    char rev_str[4];

    /*! Device name (e.g. "BCM56801"). */
    const char *name;

    /*! Device type string (e.g. "bcm56800_a0"). */
    const char *type_str;

    /*! Device type (enumeration of supported devices). */
    bcmdrd_dev_type_t type;

    /*! I/O access functions and info */
    bcmdrd_hal_io_t io;

    /*! DMA access functions */
    bcmdrd_hal_dma_t dma;

    /*! Interrupt control API */
    bcmdrd_hal_intr_t intr;

    /*! Chip information structure. */
    const bcmdrd_chip_info_t *chip_info;
#ifndef PKTIO_IMPL
    /*! Chip profile (typically NULL if base device). */
    bcmdrd_chip_profile_t *chip_profile;

    /*! Port information array. */
    const bcmdrd_port_info_t *port_info;

    /*! Bit map of valid physical ports in the device. */
    bcmdrd_pbmp_t valid_ports;

    /*! Bit map of valid pipes in the device. */
    bcmdrd_pipe_data_t *valid_pipes;

    /*! Pipeline bypass mode (device-specific). */
    uint32_t bypass_mode;

    /*! Operation mode (device-specific). */
    uint32_t opmode;

    /*! Whether device bus has been initialized. */
    bool bus_initialized;

    /*! Hash-based lookup for chip symbol modifiers. */
    shr_cht_t *chip_mod_cht;

    /*! Whether physical block has any valid ports. */
    bool *blk_has_valid_ports;
#endif
} bcmdrd_dev_t;

/*!
 * \brief Get DRD device handle.
 *
 * \param [in] unit Unit number.
 *
 * \return DRD device handle or NULL if not found.
 */
extern bcmdrd_dev_t *
bcmdrd_dev_get(int unit);

#endif /* BCMDRD_INTERNAL_H */
