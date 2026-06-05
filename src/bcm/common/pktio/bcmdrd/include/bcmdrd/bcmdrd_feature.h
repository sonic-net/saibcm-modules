/*! \file bcmdrd_feature.h
 *
 * DRD feature interface.
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

#ifndef BCMDRD_FEATURE_H
#define BCMDRD_FEATURE_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <sal/sal_types.h>
#endif
#include <bcmdrd/bcmdrd_feature_enum.h>

/*!
 * \brief Set a device feature as present.
 *
 * \param [in] unit Unit number.
 * \param [in] feature Feature to enable for this device.
 */
extern void
bcmdrd_feature_enable(int unit, bcmdrd_feature_t feature);

/*!
 * \brief Set a device feature as not present.
 *
 * Passing feature \c BCMDRD_FT_ALL will clear all features.
 *
 * \param [in] unit Unit number.
 * \param [in] feature Feature to disable for this device.
 */
extern void
bcmdrd_feature_disable(int unit, bcmdrd_feature_t feature);

/*!
 * \brief Check if a feature is present for a device.
 *
 * \param [in] unit Unit number
 * \param [in] feature Feature to check for.
 *
 * \return true if feature is present, otherwise false.
 */
extern bool
bcmdrd_feature_enabled(int unit, bcmdrd_feature_t feature);

/*!
 * \brief Get the name of a feature.
 *
 * \param [in] unit Unit number
 * \param [in] feature Feature to get the name for.
 *
 * \return Name of the specified feature.
 */
extern const char *
bcmdrd_feature_name(int unit, bcmdrd_feature_t feature);

/*!
 * \brief Test if running on real hardware.
 *
 * Convenience function to test that we are not running on a simulated
 * or emulated device.
 *
 * \param [in] unit Unit number.
 *
 * \retval true Running on real hardware.
 * \retval false Running on a simulated or emulated device.
 */
bool
bcmdrd_feature_is_real_hw(int unit);

/*!
 * \brief Test if running on a simulated device.
 *
 * Convenience function to test if we are running on a simulated
 * device.
 *
 * \param [in] unit Unit number.
 *
 * \retval true Running on a simulated device.
 * \retval false Running on real hardware or an emulated device.
 */
bool
bcmdrd_feature_is_sim(int unit);

#endif /* BCMDRD_FEATURE_H */
