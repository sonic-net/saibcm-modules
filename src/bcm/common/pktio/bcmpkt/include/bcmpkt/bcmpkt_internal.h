/*! \file bcmpkt_internal.h
 *
 * Internal common head file.
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

#ifndef BCMPKT_INTERNAL_H
#define BCMPKT_INTERNAL_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <sal/sal_types.h>
#include <shr/shr_types.h>
#include <shr/shr_pb.h>
#endif

#include <bcmdrd/bcmdrd_dev.h>
#include <bcmpkt/bcmpkt_net.h>
#include <bcmpkt/bcmpkt_pmd.h>
#include <bcmpkt/bcmpkt_pmd_internal.h>

/*! Get a field from a DOP Trace buffer. */
typedef uint32_t (*bcmpkt_dop_field_get_f)(uint32_t *data_in,
                                           uint32_t *data_out,
                                           uint32_t data_out_len);

/*! Get a field array from a DOP Trace buffer. */
typedef uint32_t (*bcmpkt_pt_to_dop_info_array_get_f)(int unit,
                                                      const char *pt_name,
                                                      const uint8_t *data,
                                                      uint32_t port_id,
                                                      uint32_t *info_cnt,
                                                      uint32_t **info);

/*! Get a field from a DOP Trace buffer. */
typedef uint32_t (*bcmpkt_pt_to_dop_info_get_f)(int unit, const char *pt_name,
                                                const uint8_t *data,
                                                uint32_t port_id,
                                                uint32_t *info);

/*! Get a DOP data buffer. */
typedef uint32_t (*bcmpkt_dop_fids_get_f)(uint32_t *fid_list,
                                          uint8_t *fid_count);

/*! Get a Trace DOP attributions. */
typedef uint32_t (*bcmpkt_dop_iget_f)(void);

/*!
 * \brief RX callback information structure.
 */
typedef struct bcmpkt_rx_cb_info_s {

    /*! Next callback info handle. */
    struct bcmpkt_rx_cb_info_s *cb_next;

    /*! Callback flags. */
    uint32_t flags;

    /*! Callback function. */
    bcmpkt_rx_cb_f cb_func;

    /*! Callback application contex. */
    void *cb_data;

    /*! True: Pending in callback. */
    bool cb_pending;

    /*! True: Pending in callback unregistering state. */
    bool cb_unreging;

} bcmpkt_rx_cb_info_t;

/*!
 * \brief Check if a registered device driver is being actively used.
 *
 * \param [in] type Device driver type.
 *
 * \retval 1 Some device is using the driver.
 * \retval 0 No device is using the driver.
 */
extern int
bcmpkt_dev_drv_inuse(bcmpkt_dev_drv_types_t type);

/*!
 * \brief Check if a registered SOCKET driver is being actively used
 *
 * \param [in] type SOCKET driver type.
 *
 * \retval 1 Some device is using the driver.
 * \retval 0 No device is using the driver.
 */
extern int
bcmpkt_socket_drv_inuse(bcmpkt_socket_drv_types_t type);

/*!
 * \brief Get device ID.
 *
 * \param [in] unit Unit number.
 * \param [out] id Device ID.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_PARAM Input len is too small.
 * \retval SHR_E_NOT_FOUND Devuce name not found.
 */
extern int
bcmpkt_dev_id_get(int unit, uint32_t *id);

/*!
 * \brief Get RCPU header's configuration handle.
 *
 * \param [in] unit Switch unit number.
 *
 * \retval RCPU header's configuration handle.
 */
extern bcmpkt_rcpu_hdr_t *
bcmpkt_rcpu_hdr(int unit);

/*!
 * \brief Suspend or resume KNET packet transmit function.
 *
 * This function is for suspend or resume packet transmission in KNET.
 *
 * \param [in] unit Switch unit number.
 * \param [in] suspend True to suspend TX packets or false to resume.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_knet_tx_suspend_set(int unit, bool suspend);

/*!
 * \brief Generate TX header.
 *
 * \param [in]  unit Unit number.
 * \param [in]  packet Packet data.
 * \param [in]  rhdr_en Enable RCPU header or not.
 *
 * \return SHR_E_NONE on success, error code otherwise.
 */
extern int
bcmpkt_dev_tx_hdr_generate(int unit,
                           bcmpkt_packet_t *packet, bool rhdr_en);

/*!
 * \brief Internal function for generating TX header for XGS devices.
 *
 * \param [in]  unit Unit number.
 * \param [in]  packet Packet data.
 * \param [in]  rhdr_en Enable RCPU header or not.
 *
 * \return SHR_E_NONE on success, error code otherwise.
 */
extern int
bcmpkt_txhdr_xgs_gen(int unit, bcmpkt_packet_t *packet, bool rhdr_en);

/*!
 * \brief Internal function for generating TX header for XFS(flex) devices.
 *
 * \param [in]  unit Unit number.
 * \param [in]  packet Packet data.
 * \param [in]  rhdr_en Enable RCPU header or not.
 *
 * \return SHR_E_NONE on success, error code otherwise.
 */
extern int
bcmpkt_txhdr_xfs_gen(int unit, bcmpkt_packet_t *packet, bool rhdr_en);

/*!
 * \brief Internal function for generating TX header for XFS + XGS devices.
 *
 * \param [in]  unit Unit number.
 * \param [in]  packet Packet data.
 * \param [in]  rhdr_en Enable RCPU header or not.
 *
 * \return SHR_E_NONE on success, error code otherwise.
 */
extern int
bcmpkt_txhdr_xfs_xgs_gen(int unit, bcmpkt_packet_t *packet, bool rhdr_en);

/*!
 * \brief Internal function for generating TX header for DNX devices.
 *
 * \param [in]  unit Unit number.
 * \param [in]  packet Packet data.
 * \param [in]  rhdr_en Enable RCPU header or not.
 *
 * \return SHR_E_NONE on success, error code otherwise.
 */
extern int
bcmpkt_txhdr_dnx_gen(int unit, bcmpkt_packet_t * packet, bool rhdr_en);

/*!
 * \brief Internal function for generating TX header for DNXF devices.
 *
 * \param [in]  unit Unit number.
 * \param [in]  packet Packet data.
 * \param [in]  rhdr_en Enable RCPU header or not.
 *
 * \return SHR_E_NONE on success, error code otherwise.
 */
extern int
bcmpkt_txhdr_dnxf_gen(int unit, bcmpkt_packet_t * packet, bool rhdr_en);
	
/*! \cond  Externs for trace driver attach functions. */
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern int _bd##_dev_drv_attach(bcmpkt_dev_t *drv);
#define BCMDRD_DEVLIST_OVERRIDE
#include <bcmdrd/bcmdrd_devlist.h>
/*! \endcond */

#endif /* BCMPKT_INTERNAL_H */
