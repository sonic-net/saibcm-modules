/*! \file ngknet_msg.h
 *
 * NGKNET messages.
 *
 * This file is intended for use by other kernel modules relying on the KNET.
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

#ifndef NGKNET_MSG_H
#define NGKNET_MSG_H

#include <lkm/ngknet_dev.h>

#define NGKNET_MSG_CMD_RX_START_INFO          (1)
#define NGKNET_MSG_CMD_DBG_LVL_INFO           (2)
#define NGKNET_MSG_CMD_SCRATCH_INFO           (3)
#define NGKNET_MSG_CMD_SCRATCH_INFO_FETCH     (4)

#define NGKNET_SCRATCH_DATA_WORDS             4
#define NGKNET_SCRATCH_DATA_BYTES             16
#define NGKNET_OAMP_PORT_MAX                  8

typedef struct ngknet_msg_rx_start_info_s {
    uint32_t flags;
#define NGKNET_F_RX_START_INFO_DELIVERED      (0x1 << 0)
#define NGKNET_F_RX_PKT_UNPARSED              (0x1 << 1)
#define NGKNET_F_RX_PKT_AI_FORMAT             (0x1 << 2)
    uint32_t enet_channels;
    uint32_t system_headers_mode;
    uint8_t ftmh_lb_key_size;
    uint8_t ftmh_stacking_ext_size;
    uint8_t udh_enabled;
    uint8_t pph_base_size;
    uint8_t pph_lif_ext_size[8];
    uint8_t udh_length_type[4];
    uint16_t oamp_system_port_0;
    uint16_t oamp_system_port_1;
    uint16_t up_mep_ingress_cpu_trap_id1;
    uint16_t up_mep_ingress_cpu_trap_id2;
    uint32_t oamp_port_number;
    uint32_t oamp_ports[NGKNET_OAMP_PORT_MAX];
    uint8_t spa_mode;
} ngknet_msg_rx_start_info_t;

typedef struct ngknet_msg_scratch_data_s {
    int filter_id;
    int spa_unit;
    uint32_t match_flags;
    uint32_t data[NGKNET_SCRATCH_DATA_WORDS];
    uint32_t mask[NGKNET_SCRATCH_DATA_WORDS];
} ngknet_msg_scratch_data_t;

#endif /* NGKNET_MSG_H */

