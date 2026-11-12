/*! \file bcmpkt_rxpmd_ext.h
 *
 * RXPMD_EXT header data access interfaces.
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

#ifndef BCMPKT_RXPMD_EXT_H
#define BCMPKT_RXPMD_EXT_H

#include <sal/sal_types.h>
#include <shr/shr_bitop.h>
#include <bcmpkt/bcmpkt_rxpmd_ext_defs.h>
#include <bcmdrd/bcmdrd_types.h>
#include <bcmlrd/bcmlrd_conf.h>
#include <bcmpkt/bcmpkt_rxpmd_ext_field.h>
#include <bcmpkt/bcmpkt_rxpmd_ext_fid.h>
#include <bcmpkt/bcmpkt_packet.h>

/*! RXPMD_EXT Header maximum size (words). */
#define BCMPKT_RXPMD_EXT_SIZE_WORDS         8

/*! RXPMD_EXT Header maximum size (bytes). */
#define BCMPKT_RXPMD_EXT_SIZE_BYTES         32

/*! RXPMD_EXT FID field supported check. */
#define BCMPKT_RXPMD_EXT_FID_SUPPORTED(_st, _f) SHR_BITGET((_st)->fbits, _f)

/*!
 * \name RXPMD_EXT Dump flags. (deprecated by BCMPKT_DUMP_F_XXX)
 * \anchor BCMPKT_RXPMD_EXT_DUMP_F_XXX
 */
/*! \{ */
/*!
 * Dump all fields contents.
 */
#define BCMPKT_RXPMD_EXT_DUMP_F_ALL         0
/*!
 * Dump non-zero field content only.
 */
#define BCMPKT_RXPMD_EXT_DUMP_F_NONE_ZERO   1
/*! \} */

/*!
 * \brief Get RXPMD_EXT's size for a given device type.
 *
 * \param [in] dev_type Device type.
 * \param [out] len Bytes of RXPMD_EXT length.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Unsupported device type or bad \c len pointer.
 * \retval SHR_E_UNAVAIL Not support RXPMD_EXT get function.
 */
extern int
bcmpkt_rxpmd_ext_len_get(bcmdrd_dev_type_t dev_type, uint32_t *len);

/*!
 * \brief Get field name for a given RXPMD_EXT field ID.
 *
 * \param [in] fid RXPMD_EXT field ID, refer to \ref BCMPKT_RXPMD_EXT_XXX.
 * \param [out] name RXPMD_EXT field name string.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_rxpmd_ext_field_name_get(int fid, char **name);

/*!
 * \brief Get field ID for a given RXPMD_EXT field name.
 *
 * \param [in] name RXPMD_EXT field name string.
 * \param [out] fid RXPMD_EXT Field ID.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_NOT_FOUND Not found the name.
 */
extern int
bcmpkt_rxpmd_ext_field_id_get(char *name, int *fid);

/*!
 * \brief Get RXPMD_EXT data from a packet.
 *
 * \param [in] packet Packet handle.
 * \param [in] ethertype Ethertype value to match.
 * \param [out] data Pointer to RXPMD_EXT data buffer.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Invalid packet or data pointer.
 * \retval SHR_E_UNAVAIL Ethertype marker does not match in packet..
 */
extern int
bcmpkt_rxpmd_ext_get(bcmpkt_packet_t *packet, uint16_t ethertype,
                     uint32_t **data);

/*!
 * \brief Remove RXPMD_EXT header from a packet.
 *
 * This function removes the RXPMD_EXT header from the packet if the ethertype
 * matches the provided value. It pulls the header bytes from the packet data.
 *
 * \param [in] packet Packet handle.
 * \param [in] ethertype Ethertype value to match.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Invalid packet pointer.
 * \retval SHR_E_UNAVAIL Ethertype marker does not match in packet.
 */
extern int
bcmpkt_rxpmd_ext_remove(bcmpkt_packet_t *packet, uint16_t ethertype);


#endif /* BCMPKT_RXPMD_EXT_H */
