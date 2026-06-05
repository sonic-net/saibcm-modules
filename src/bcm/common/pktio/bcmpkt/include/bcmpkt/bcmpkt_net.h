/*! \file bcmpkt_net.h
 *
 * Interfaces for transmitting and receiving packets for the host CPU.
 *
 * On transmitting, the user calls \ref bcmpkt_tx_f to send out a packet from a
 * network interface. The bcmpkt_txpmd_* interfaces could be used for defining
 * specific destination, e.g. an egress queue of a local port.
 *
 * On receiving, the user registers application's callback onto a netif.
 * When a packet received, the callback will be called by RX handler to receive
 * the packet. Packet structure has a encoded packet metadata for forwarding
 * information. The user calls bcmpkt_rxpmd_* interfaces to parsing forwarding
 * informaiton for the packet. Refer \ref bcmpkt_rxpmd.h for packet metadata
 * APIs.
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

#ifndef BCMPKT_NET_H
#define BCMPKT_NET_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <sal/sal_types.h>
#include <shr/shr_types.h>
#endif
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_txpmd.h>
#include <bcmpkt/bcmpkt_lbhdr.h>
#include <bcmpkt/bcmpkt_higig_defs.h>
#include <bcmpkt/bcmpkt_hg3.h>
#include <bcmpkt/bcmpkt_socket.h>
#include <bcmpkt/bcmpkt_rcpu_hdr.h>
#include <bcmpkt/bcmpkt_packet.h>
#include <bcmdrd/bcmdrd_types.h>

/*! Loopback Header maximum size. (words) */
#define BCMPKT_LBHDR_SIZE_WORDS      4

/*!
 * NET driver types.
 */
typedef enum bcmpkt_net_drv_types_e {
    BCMPKT_NET_DRV_T_NONE = BCMPKT_DEV_DRV_T_NONE, /*! No NET driver. */
    BCMPKT_NET_DRV_T_AUTO = BCMPKT_DEV_DRV_T_AUTO, /*! Reserved. */
    BCMPKT_NET_DRV_T_UNET = BCMPKT_DEV_DRV_T_UNET, /*! User land DMA driver. */
    BCMPKT_NET_DRV_T_TPKT = BCMPKT_SOCKET_DRV_T_TPKT,/*! Packet_mmap NET. */
    BCMPKT_NET_DRV_T_RAWSOCK = BCMPKT_SOCKET_DRV_T_RAWSOCK,/*! Raw socket NET. */
    BCMPKT_NET_DRV_T_COUNT /*! Must be end */
} bcmpkt_net_drv_types_t;

/*!
 * \brief Packet receive callback function type.
 *
 * Callback function type for applications using RX facility.
 *
 * The user defines its callback function and register it onto a network
 * interface to receive its packets.
 *
 * The 'packet' buffer should not be freed in application software. If the
 * would like to use the packet out of its callback, the packet data should be
 * copied in callback function for application usage.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] packet Packet handle.
 * \param [in] cb_data Application-provided context.
 *
 * \return SHR_E_XXX Leave for future.
 */
typedef int (*bcmpkt_rx_cb_f)(int unit, int netif_id, bcmpkt_packet_t *packet,
                               void *cb_data);

/*!
 * \brief Packet receive callback register function.
 *
 * This function is for upper applications to regiter their callback functions
 * onto RX packet handler to receive its packets.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] flags Reserved for future.
 * \param [in] cb_func Packet receive callback function.
 * \param [in] cb_data Application-provided context.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_BADID The netif_id is invalid or doesn't support SOCKET.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_MEMORY Allocate buffer failed.
 * \retval SHR_E_EXISTS Callback already exists.
 */
typedef int (*bcmpkt_rx_register_f)(int unit, int netif_id, uint32_t flags,
                                    bcmpkt_rx_cb_f cb_func, void *cb_data);

/*!
 * \brief Packet receive callback deregister function.
 *
 * Remove callback function from the network interface to disable receive
 * packets from it.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] cb_func Packet receive callback function.
 * \param [in] cb_data Application-provided context.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_BADID Netif_id is invalid or doesn't support SOCKET.
 * \retval SHR_E_PARAM Check parameters failed.
 */
typedef int (*bcmpkt_rx_unregister_f)(int unit, int netif_id,
                                      bcmpkt_rx_cb_f cb_func, void *cb_data);

/*!
 * \brief Packet transmit function.
 *
 * This function is for application to send out a packet through Broadcom Packet
 * API.
 *
 * If a packet is to be sent to a specific local port and/or go through Higig
 * IPIPE, the metadata and/or Higig header should be configured into
 * \ref bcmpkt_packet_t.pmd (\ref BCMPKT_FWD_T_NORMAL type) or encapsulated
 * into RCPU/Higig header (\ref BCMPKT_FWD_T_RAW type).
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface number.
 * \param [in] packet Packet handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_BADID Netif_id is invalid or doesn't support SOCKET.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Transmit failed.
 */
typedef int (*bcmpkt_tx_f)(int unit, int netif_id, bcmpkt_packet_t *packet);

/*!
 * \brief NET operation vector.
 */
typedef struct bcmpkt_net_s {

    /*! initialized flag: 0 - uninitialized 1 - initialized. */
    int initialized;

    /*! Driver name, such as "TPacket". */
    char driver_name[128];

    /*! SOCKET driver type. */
    bcmpkt_net_drv_types_t driver_type;

    /*! Register RX callback. */
    bcmpkt_rx_register_f rx_register;

    /*! Unregister RX callback. */
    bcmpkt_rx_unregister_f rx_unregister;

    /*! Transmit function. */
    bcmpkt_tx_f tx;

} bcmpkt_net_t;

/*!
 * \brief NET driver register function.
 *
 *
 * \param [in] net_drv Network driver handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_CONFIG Invalid driver.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_net_drv_register(bcmpkt_net_t *net_drv);

/*!
 * \brief Unregister NET driver.
 *
 * \param [in] type NET driver type.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_BUSY The driver is in using.
 */
extern int
bcmpkt_net_drv_unregister(bcmpkt_net_drv_types_t type);

/*!
 * \brief Get NET driver's type.
 *
 * \param [in] unit Switch unit number.
 * \param [out] type NET driver type.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 */
extern int
bcmpkt_net_drv_type_get(int unit, bcmpkt_net_drv_types_t *type);

/*!
 * \brief Direct forwarding port set function.
 *
 * Configure a destination port for bypass CPU port IPIPE and forwarding the
 * packet to the specific port directly. This function will set TXPMD (SOBMH)
 * for the forwarding.
 *
 * \param [in] dev_type Switch device type.
 * \param [in] port Packet destination port.
 * \param [in,out] packet Packet handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_INTERNAL Internal failure.
 */
extern int
bcmpkt_fwd_port_set(bcmdrd_dev_type_t dev_type, int port,
                    bcmpkt_packet_t *packet);

/*!
 * \brief Packet forwarding type set function.
 *
 * Packet's default type is NORMAL. If the user want to change the type, this
 * API is used for the purpose.
 *
 * \param [in] type Packet forward type.
 * \param [in,out] packet Packet handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_fwd_type_set(bcmpkt_fwd_types_t type, bcmpkt_packet_t *packet);

/*!
 * \brief Packet receive callback register function.
 *
 * This function is for upper applications to regiter their callback functions
 * onto RX packet handler to receive its packets.
 *
 * The 'netif_id' is not used for UNET driver and would be ignored.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] flags Reserved for future.
 * \param [in] cb_func Packet receive callback function.
 * \param [in] cb_data Application-provided context.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_BADID The netif_id is invalid or doesn't support SOCKET.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_MEMORY Allocate buffer failed.
 * \retval SHR_E_EXISTS Callback already exists.
 */
extern int
bcmpkt_rx_register(int unit, int netif_id, uint32_t flags,
                   bcmpkt_rx_cb_f cb_func, void *cb_data);

/*!
 * \brief Packet receive callback deregister function.
 *
 * Remove callback function from the network interface to disable receive
 * packets from it.
 *
 * The 'netif_id' is not used for UNET driver and would be ignored.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] cb_func Packet receive callback function.
 * \param [in] cb_data Application-provided context.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_BADID Netif_id is invalid or doesn't support SOCKET.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_rx_unregister(int unit, int netif_id, bcmpkt_rx_cb_f cb_func,
                     void *cb_data);

/*!
 * \brief Packet transmit function.
 *
 * This function is for application to send out a packet through Broadcom Packet
 * API.
 *
 * If a packet is to be sent to a specific local port and/or go through Higig
 * IPIPE, the metadata and/or Higig header should be configured into
 * \ref bcmpkt_packet_t.pmd (\ref BCMPKT_FWD_T_NORMAL type) or encapsulated
 * into RCPU/Higig header (\ref BCMPKT_FWD_T_RAW type).
 *
 * For UNET driver mode, 'netif_id' is TX DMA queue index (starts from 1).
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface number.
 * \param [in] packet Packet handle.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_BADID Netif_id is invalid or doesn't support SOCKET.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Transmit failed.
 */
extern int
bcmpkt_tx(int unit, int netif_id, bcmpkt_packet_t *packet);

/*!
 * \brief Suspend or resume packet transmit function.
 *
 * This function is for suspend or resume sending out a packet through
 * bcmpkt_tx API.
 *
 * \param [in] unit Switch unit number.
 * \param [in] suspend True to suspend TX packets or false to resume.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_tx_suspend_set(int unit, bool suspend);

/*!
 * \brief Get the suspend status of packet transmit function.
 *
 * This function is for getting the suspend status of packet transmission.
 *
 * \param [in] unit Switch unit number.
 * \param [out] suspend True for suspended or false for not suspended.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_tx_suspend_get(int unit, bool *suspend);

#endif /* BCMPKT_NET_H */
