/*! \file bcmpkt_packet.h
 *
 * Packet definition for host CPU transmitting and receiving.
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

#ifndef BCMPKT_PACKET_H
#define BCMPKT_PACKET_H

#include <bcmpkt/bcmpkt_rcpu_hdr.h>

/*!
 * This size is for \ref bcmpkt_packet_t.pmd data.
 * Including RXPMD, TXPMD, Higig, and Loopback Header.
 * RXPMD use RX DCB size, and others use TX DCB Module header size.
 * Real size of them will be take care in API.
 */
#define BCMPKT_PMD_SIZE_BYTES       (BCMPKT_RCPU_RXPMD_SIZE + \
                                     BCMPKT_RCPU_TX_MH_SIZE * 3)

/*! The bcmpkt_packet_t.pmd.data size. (number of words) */
#define BCMPKT_PMD_SIZE_WORDS       (BCMPKT_PMD_SIZE_BYTES / 4)

/*!
 * This space is reserved for (o) optional components.
 *
 *  (o) RCPU header
 *  (o) Tx meta data (Tx PMD)
 *  (o) Loopback header
 *  (o) HiGig header *
 */
#define BCMPKT_TX_HDR_RSV           BCMPKT_RCPU_MAX_ENCAP_SIZE

/*! Packet data handle. */
#define BCMPKT_PACKET_DATA(_pkt)    (_pkt)->data_buf->data

/*! Packet data length. */
#define BCMPKT_PACKET_LEN(_pkt)     (_pkt)->data_buf->data_len

/*!
 * Packet forwarding types.
 * The \ref bcmpkt_packet_t.pmd defines normal packet forwarding and
 * encapsulation information. For BCMPKT_PACKET_T_NORMAL type, SDk will refer
 * \ref bcmpkt_packet_t.pmd for specific forwarding and Higig encapsulation.
 * If BCMPKT_FWD_T_RAW type is set, SDK will ignore \ref bcmpkt_packet_t.pmd.
 * In this case, the application user should take care of RCPU header and Higig
 * header because call \ref bcmpkt_net_t.tx to send out the packet.
 * The BCMPKT_FWD_T_RAW is normally used for debugging purpose.
 */
typedef enum bcmpkt_fwd_types_e {
    /*! Normal Packet. */
    BCMPKT_FWD_T_NORMAL,
    /*! Raw packet, may include RCPU header, TXPMD and/or Higig header. */
    BCMPKT_FWD_T_RAW,
     /*! Must be end */
    BCMPKT_FWD_T_COUNT
} bcmpkt_fwd_types_t;

/*!
 * Packet metadata information.
 */
typedef struct bcmpkt_pmd_s {

    /*! RX Packet metadata handle. */
    uint32_t *rxpmd;

    /*! TX Packet metadata handle. */
    uint32_t *txpmd;

    /*! Higig handle. */
    uint32_t *higig;

    /*! Loopback Header handle. */
    uint32_t *lbhdr;

    /*! Headers' data. */
    uint32_t data[BCMPKT_PMD_SIZE_WORDS];

} bcmpkt_pmd_t;

/*!
 * Packet data buffer information.
 *
 * The 'head' is the packet data buffer's pointer. The space between 'head' and
 * 'data' can be used for packet data content adjustment and/or RCPU header,
 * TXPMD and Higig encapsulation.
 *
 * The \c ref_count is used for clone function. It increases for each packet
 * clone function called, and decreases for each data buffer free called. When
 * \c ref_count = 0, free the buffer.
 */
typedef struct bcmpkt_data_buf_s {

    /*! Packet buffer head pointer. */
    uint8_t *head;

    /*! Packet buffer size (start from head). */
    uint32_t len;

    /*! Packet data pointer. */
    uint8_t *data;

    /*! Packet data size, unit is byte. */
    uint32_t data_len;

    /*! Number of packets using this data buffer. */
    int ref_count;
} bcmpkt_data_buf_t;

/*!
 * \name Packet flags.
 * \anchor BCMPKT_PACKET_F_XXX
 */
/*! \{ */
/*! Do not pad runt TX packet. */
#define BCMPKT_PACKET_F_TX_NO_PAD      (1 << 0)
/*! \} */

/*!
 * \brief Packet structure.
 *
 * \c unit is for per device DMA buffer management.
 * \c type is packet forwarding type.
 * \c pmd saves packet metadata information.
 * \c data saves data buffer information.
 */
typedef struct bcmpkt_packet_s {

    /*! Point to next packet in the list. */
    struct bcmpkt_packet_s *next;

    /*! Point to previous packet in the list. */
    struct bcmpkt_packet_s *prev;

    /*! Switch unit number. */
    int unit;

    /*! Flags, refer to \ref BCMPKT_PACKET_F_XXX. */
    uint32_t flags;

    /*! Packet forwarding type, refer to \ref bcmpkt_fwd_types_t. */
    uint32_t type;

    /*! Packet metadata information. */
    bcmpkt_pmd_t pmd;

    /*! Packet data buffer information. */
    bcmpkt_data_buf_t *data_buf;

} bcmpkt_packet_t;

#endif /* BCMPKT_PACKET_H */

