/*! \file ngknet_parser.h
 *
 * Definitions and APIs declaration for KNET parser.
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

#ifndef NGKNET_PARSER_H
#define NGKNET_PARSER_H

#include "ngknet_adapter.h"

extern int ngknet_rx_parser_debug_set(
    int debug_lvl);

extern int ngknet_rx_parser(
    struct adapter_dev *adev,
    struct sk_buff *skb);

extern bool ngknet_rx_parser_scratch_data_match(
    struct adapter_dev *adev,
    struct sk_buff *skb,
    ngknet_filter_t *filt);
#endif /* NGKNET_PARSER_H */

