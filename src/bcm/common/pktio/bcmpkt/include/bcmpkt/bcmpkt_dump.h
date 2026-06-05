/*! \file bcmpkt_dump.h
 *
 * \brief Packet dump functions.
 *
 * The defintions are kept separate to minimize the header file
 * dependencies for the stand-alone PMD library.
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

#ifndef BCMPKT_DUMP_H
#define BCMPKT_DUMP_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <shr/shr_pb.h>
#endif
#include <bcmdrd/bcmdrd_types.h>
#include <bcmlrd/bcmlrd_conf.h>
#include <bcmpkt/bcmpkt_packet.h>

/*!
 * \name BCMPKT dump flags.
 * \anchor BCMPKT_DUMP_F_XXX
 */
/*! \{ */
/*! Dump all fields contents. (deprecated) */
#define BCMPKT_DUMP_F_ALL           0
/*! Dump non-zero field content only. */
#define BCMPKT_DUMP_F_NONZERO       (0x1 << 0)
/*! \} */

/*!
 * \brief Dump raw data buffer into \c pb.
 *
 * \param [in,out] pb Print buffer handle.
 * \param [in] data Data to be printed.
 * \param [in] size print bytes.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_PARAM Invalid type.
 */
extern int
bcmpkt_data_dump(shr_pb_t *pb, const uint8_t *data, int size);

/*!
 * \brief Dump packet data buffer into \c pb.
 *
 * \param [in,out] pb Print buffer handle.
 * \param [in] dbuf Data buffer to be printed.
 *
 * \retval None.
 */
extern void
bcmpkt_data_buf_dump(bcmpkt_data_buf_t *dbuf, shr_pb_t *pb);

/*!
 * \brief Dump all supported RXPMD fields into \c pb.
 *
 * If view_name is given, dump common fields and the fields belonging to the
 * view. If view_name is NULL, dump common fields and the fields of all view's.
 * If view_name is unknown, only dump common fields.
 *
 * \param [in] dev_type Device type.
 * \param [in] view_name RXPMD view name.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rxpmd_field_list_dump(bcmdrd_dev_type_t dev_type, char *view_name,
                             shr_pb_t *pb);

/*!
 * \brief Dump RXPMD content into \c pb.
 *
 * This function is used for dumping the content of an RXPMD. If the
 * BCMPKT_DUMP_F_NONZERO is set, only dump non-zero fields.
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd RXPMD handle.
 * \param [in] flags Refer to \ref BCMPKT_RXPMD_DUMP_F_XXX.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rxpmd_dump(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd, uint32_t flags,
                  shr_pb_t *pb);

/*!
 * \brief Dump RX reasons into \c pb.
 *
 * \param [in] dev_type Device type.
 * \param [in] rxpmd RXPMD handle.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_UNAVAIL Not support Reason.
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rx_reason_dump(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd,
                      shr_pb_t *pb);

/*!
 * \brief Dump all supported TXPMD fields into \c pb.
 *
 * If view_name is given, dump common fields and the fields belonging to the
 * view. If view_name is NULL, dump common fields and the fields of all view's.
 * If view_name is unknown, only dump common fields.
 *
 * \param [in] dev_type Device type.
 * \param [in] view_name TXPMD view name.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_txpmd_field_list_dump(bcmdrd_dev_type_t dev_type, char *view_name,
                             shr_pb_t *pb);

/*!
 * \brief Dump TXPMD content into \c pb.
 *
 * This function is used for dumping the content of a TXPMD. If the
 * BCMPKT_DUMP_F_NONZERO is set, only dump non-zero fields.
 *
 * \param [in] dev_type Device type.
 * \param [in] txpmd TXPMD handle.
 * \param [in] flags Defined as \ref BCMPKT_TXPMD_DUMP_F_XXX.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_txpmd_dump(bcmdrd_dev_type_t dev_type, uint32_t *txpmd, uint32_t flags,
                  shr_pb_t *pb);

/*!
 * \brief Dump all supported LBHDR fields into \c pb.
 *
 * If view_name is given, dump common fields and the fields belonging to the
 * view. If view_name is NULL, dump common fields and the fields of all view's.
 * If view_name is unknown, only dump common fields.
 *
 * \param [in] dev_type Device type.
 * \param [in] view_name LBHDR view name.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_lbhdr_field_list_dump(bcmdrd_dev_type_t dev_type, char *view_name,
                             shr_pb_t *pb);

/*!
 * \brief Dump LBHDR content into \c pb.
 *
 * This function is used for dumping the content of a LBHDR. If the
 * BCMPKT_DUMP_F_NONZERO is set, only dump non-zero fields.
 *
 * \param [in] dev_type Device type.
 * \param [in] lbhdr LBHDR handle.
 * \param [in] flags Defined as \ref BCMPKT_LBHDR_DUMP_F_XXX.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_lbhdr_dump(bcmdrd_dev_type_t dev_type, uint32_t *lbhdr, uint32_t flags,
                  shr_pb_t *pb);

/*!
 * \brief Dump all supported flexhdr fields into \c pb.
 *
 * If view_name is given, dump common fields and the fields belonging to the
 * view. If view_name is NULL, dump common fields and the fields of all view's.
 * If view_name is unknown, only dump common fields.
 *
 * \param [in] variant variant type.
 * \param [in] hid flexhdr ID.
 * \param [in] view_name flexhdr view name.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_flexhdr_field_list_dump(bcmlrd_variant_t variant, uint32_t hid,
                               char *view_name, shr_pb_t *pb);

/*!
 * \brief Dump flexhdr content into \c pb.
 *
 * This function is used for dumping the content of a flexhdr. If the
 * BCMPKT_DUMP_F_NONZERO is set, only dump non-zero fields.
 *
 * \param [in] variant Variant type.
 * \param [in] hid flexhdr ID.
 * \param [in] flexhdr flexhdr handle.
 * \param [in] flags Defined as \ref BCMPKT_DUMP_F_XXX.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_flexhdr_dump(bcmlrd_variant_t variant, uint32_t hid, uint32_t *flexhdr,
                    uint32_t flags, shr_pb_t *pb);

/*!
 * \brief Dump RXPMD_FLEX content into \c pb.
 *
 * This function is used for dumping the content of an RXPMD_FLEX. If the
 * BCMPKT_DUMP_F_NONZERO is set, only dump non-zero fields.
 *
 * \param [in] variant Variant type.
 * \param [in] rxpmd_flex RXPMD_FLEX handle.
 * \param [in] profile Flexible data profile.
 * \param [in] flags Refer to \ref BCMPKT_DUMP_F_XXX.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rxpmd_flex_dump(bcmlrd_variant_t variant, uint32_t *rxpmd_flex,
                       uint32_t profile, uint32_t flags, shr_pb_t *pb);

/*!
 * \brief Dump RXPMD_FLEX content into \c pb.
 *
 * This function is used for dumping the content of an RXPMD_FLEX. If the
 * BCMPKT_DUMP_F_NONZERO is set, only dump non-zero fields.
 *
 * \param [in] unit Device unit.
 * \param [in] rxpmd_flex RXPMD_FLEX handle.
 * \param [in] profile Flexible data profile.
 * \param [in] flags Refer to \ref BCMPKT_DUMP_F_XXX.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rxpmd_device_flex_dump(int unit, uint32_t *rxpmd_flex,
                              uint32_t profile, uint32_t flags, shr_pb_t *pb);

/*!
 * \brief Dump RX reasons into \c pb.
 *
 * \param [in] variant Variant type.
 * \param [in] rxpmd_flex RXPMD_FLEX handle.
 * \param [out] pb Print buffer handle.
 *
 * \retval SHR_E_NONE success
 * \retval SHR_E_PARAM Check parameter failed
 * \retval SHR_E_UNAVAIL Not support Reason.
 * \retval SHR_E_INTERNAL API internal error.
 */
extern int
bcmpkt_rxpmd_flex_reason_dump(bcmlrd_variant_t variant,
                              uint32_t *rxpmd_flex, shr_pb_t *pb);

#endif /* BCMPKT_DUMP_H */
