/*! \file bcmpkt_rxpmd_ext_field.h
 *
 * RXPMD_EXT header field api's.
 *
 */
/*
 *
 * Copyright 2018-2026 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef BCMPKT_RXPMD_EXT_FIELD_H
#define BCMPKT_RXPMD_EXT_FIELD_H


/*!
 * \brief Get value from an RXPMD_EXT field.
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd_ext RXPMD_EXT handle.
 * \param [in] fid RXPMD_EXT field ID, refer to \ref BCMPKT_RXPMD_EXT_XXX.
 * \param [out] val Field value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_rxpmd_ext_field_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd_ext,
                        int fid, uint32_t *val);

/*!
 * \brief Set value into an RXPMD_EXT field.
 *
 * \param [in] dev_type Device type.
 * \param [in,out] rxpmd_ext RXPMD_EXT handle.
 * \param [in] fid RXPMD_EXT field ID, refer to \ref BCMPKT_RXPMD_EXT_XXX.
 * \param [in] val Set value.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_UNAVAIL Not support the field.
 */
extern int
bcmpkt_rxpmd_ext_field_set(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd_ext,
                        int fid, uint32_t val);

#endif /* BCMPKT_RXPMD_EXT_FIELD_H */
