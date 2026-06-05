/*! \file bcmdrd.h
 *
 * External BCMDRD Device API.
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

#ifndef BCMDRD_H
#define BCMDRD_H

#include <bcmdrd/bcmdrd_types.h>
#include <bcmdrd/bcmdrd_hal.h>

/*!
 * \brief Device identification structure.
 *
 * This information defines a specific device variant, and the
 * information used to configure device drivers and features.
 */
typedef struct bcmdrd_dev_id_s {

    /*! Vendor ID (typically PCI vendor ID.) */
    uint16_t vendor_id;

    /*! Device ID (typically PCI device ID.) */
    uint16_t device_id;

    /*! Device revision (used to determine device features.) */
    uint16_t revision;

    /*! Additional identification (in case of ambiguous device IDs.) */
    uint16_t model;

} bcmdrd_dev_id_t;

/*! Device license type. */
typedef enum bcmdrd_dev_license_type_e {

    /*! HLA (Hardware License Authenticator). */
    BCMDRD_DEV_LICENSE_TYPE_HLA = 0,

    /*! Number of license types. */
    BCMDRD_DEV_LICENSE_TYPE_COUNT

} bcmdrd_dev_license_type_t;

/*!
 * \brief Device license structure.
 *
 * This structure contains pointers to a license file and an optional
 * secure application image. The license file may contain hardware license
 * (which allows hardware features to be enabled) or software license
 * (which allows the secure application to be executed). If an application
 * image is specified, the license file must contain the corresponding
 * software license. The application will be automatically executed once
 * the license file is authenticated. Optional arguments to be used
 * by the application can also be supplied in the structure.
 */
typedef struct bcmdrd_dev_license_info_s {

    /*! License type. */
    bcmdrd_dev_license_type_t license_type;

    /*! Size of the license file in bytes. */
    size_t license_size;

    /*! Pointer to the license file byte array. */
    void *license_data;

    /*! Size of the secure application image in bytes. */
    size_t appl_size;

    /*! Pointer to the secure application image byte array. */
    void *appl_data;

    /*! Number of the secure application argument words. */
    size_t appl_argc;

    /*! Pointer to the secure application argument word array. */
    uint32_t *appl_argv;

} bcmdrd_dev_license_info_t;

/*!
 * \brief Check if a device type is supported.
 *
 * \param [in] id Device ID structure.
 *
 * \return true if supported, otherwise false.
 */
extern bool
bcmdrd_dev_supported(bcmdrd_dev_id_t *id);

/*!
 * \brief Destroy a device.
 *
 * Remove a device from the DRD and free all associated resources.
 *
 * \param [in] unit Unit number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_INIT Device already destroyed (or never created).
 */
extern int
bcmdrd_dev_destroy(int unit);

/*!
 * \brief Create a device.
 *
 * Create a device in the DRD. This is a mandatory operation before
 * any other operation can be performed on the device.
 *
 * If a negative unit number is passed in, then the first available
 * unit number will be used. If no more unit numbers are available,
 * the function will return SHR_E_FULL.
 *
 * \param [in] unit Unit number.
 * \param [in] id Device ID structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_FULL No available unit numbers.
 * \retval SHR_E_EXIST Device with this unit number already exists.
 * \retval SHR_E_NOT_FOUND Device type not supported.
 */
extern int
bcmdrd_dev_create(int unit, bcmdrd_dev_id_t *id);

/*!
 * \brief Set first auto-unit number.
 *
 * Specify the first unit number to assign when -1 is passed as the
 * unit number to \ref bcmdrd_dev_create. This is mainly intended as a
 * tool for testing non-zero unit numbers on a single-unit system.
 *
 * \param [in] unit First auto-unit number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 */
extern int
bcmdrd_dev_first_auto_unit_set(int unit);

/*!
 * \brief Check if device exists.
 *
 * Check if a device has been created.
 *
 * \param [in] unit Unit number.
 *
 * \return true if device exists, otherwose false.
 */
extern bool
bcmdrd_dev_exists(int unit);

/*!
 * \brief Get device type.
 *
 * Get device type, which is an enumeration of all supported devices.
 *
 * \param [in] unit Unit number.
 *
 * \retval Device type.
 */
extern bcmdrd_dev_type_t
bcmdrd_dev_type(int unit);

/*!
 * \brief Get device type as a string.
 *
 * Get the device type as an ASCII string. If device does not exist,
 * an empty string ("") is returned.
 *
 * The device type string corresponds to the device type returned by
 * \ref bcmdrd_dev_type.
 *
 * \param [in] unit Unit number.
 *
 * \return Pointer to base device name.
 */
extern const char *
bcmdrd_dev_type_str(int unit);

/*!
 * \brief Get device revision string.
 *
 * Get a device revision string for use in the CLI. If device does not
 * exist, an empty string ("") is returned.
 *
 * \param [in] unit Unit number.
 *
 * \return Pointer to device revision string.
 */
extern const char *
bcmdrd_dev_rev_str(int unit);

/*!
 * \brief Get device name.
 *
 * Get the official device (SKU) name. If device does not exist, an
 * empty string ("") is returned.
 *
 * \param [in] unit Unit number.
 *
 * \return Pointer to device name.
 */
extern const char *
bcmdrd_dev_name(int unit);

/*!
 * \brief Get device identification information.
 *
 * \param [in] unit Unit number.
 * \param [out] id Pointer to device identification structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_NOT_FOUND Device type not supported.
 */
extern int
bcmdrd_dev_id_get(int unit, bcmdrd_dev_id_t *id);

/*!
 * \brief Get ID structure for a given device name.
 *
 * Given a device name string, this API will look for a matching ID
 * structure in the list of supported devices.
 *
 * Mainly intended for testing and debugging.
 *
 * \param [in] dev_name Device name, e.g. "bcm56800_a0".
 * \param [out] id ID structure to be filled
 *
 * \retval SHR_E_NONE Match was found and ID structure was filled.
 * \retval SHR_E_NOT_FOUND No match was found.
 */
extern int
bcmdrd_dev_id_from_name(const char *dev_name, bcmdrd_dev_id_t *id);

/*!
 * \brief Check is the device is variant SKU.
 *
 * \param [in] unit Unit number.
 *
 * \retval true The unit is variant SKU.
 * \retval false The unit is base SKU.
 */
extern bool
bcmdrd_dev_is_variant_sku(int unit);

/*!
 * \brief Assign I/O resources to a device.
 *
 * I/O resources can be assigned either as one or more physical
 * addresses of memory-mappable register windows, or as a set of
 * system-provided register access functions.
 *
 * If physical I/O memory addresses are provided, then functions for
 * mapping and unmapping I/O registers must be provided as well.
 *
 * \param [in] unit Unit number.
 * \param [in] io I/O reousrce structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_NOT_FOUND Device does not exist.
 */
extern int
bcmdrd_dev_hal_io_init(int unit, bcmdrd_hal_io_t *io);

/*!
 * \brief Retrieve configured I/O resources for a device.
 *
 * Read back the I/O resource structure supplied via \ref
 * bcmdrd_dev_hal_io_init.
 *
 * This API is mainly for diagnostic purposes.
 *
 * \param [in] unit Unit number.
 * \param [out] io I/O reousrce structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_NOT_FOUND Device does not exist.
 */
extern int
bcmdrd_dev_hal_io_get(int unit, bcmdrd_hal_io_t *io);

/*!
 * \brief Assign DMA resources to a device.
 *
 * DMA resources are required for a number of device operations, and
 * to make this possible, the system must provide functions for
 * allocating and freeing DMA memory.
 *
 * \param [in] unit Unit number.
 * \param [in] dma DMA resource structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_NOT_FOUND Device does not exist.
 */
extern int
bcmdrd_dev_hal_dma_init(int unit, bcmdrd_hal_dma_t *dma);

/*!
 * \brief Assign interrupt control API to a device.
 *
 * Allow the SDK to connect an interrupt handler to a hardwrae
 * interrupt.
 *
 * \param [in] unit Unit number.
 * \param [in] intr Interrupt control API structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_NOT_FOUND Device does not exist.
 */
extern int
bcmdrd_dev_hal_intr_init(int unit, bcmdrd_hal_intr_t *intr);

/*!
 * \brief Add device license.
 *
 * A device license is used to enable optional switch features and
 * is applied during switch initialization.
 *
 * \param [in] unit Unit number.
 * \param [in] license Device license structure.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_PARAM Invalid license data.
 * \retval SHR_E_MEMORY Insufficient memory to store license.
 * \retval SHR_E_RESOURCE Maximum number of licenses exceeded.
 * \retval SHR_E_UNAVAIL Unsupported license type.
 */
extern int
bcmdrd_dev_license_add(int unit, bcmdrd_dev_license_info_t *license);

#endif /* BCMDRD_H */
