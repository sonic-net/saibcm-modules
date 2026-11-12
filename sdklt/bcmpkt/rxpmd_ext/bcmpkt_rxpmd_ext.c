/*! \file bcmpkt_rxpmd_ext.c
 *
 * RXPMD_EXT header access interfaces.
 *
 * RXPMD_EXT is a header defined to accommodate an extension to the standard
 * Receive Packet Metadata (RXPMD).
 *
 * The core difference is:
 *
 * RXPMD refers to the original, standard set of packet metadata at RX for a
 * device.
 * RXPMD_EXT is an auxiliary, separate metadata header that contains
 * extended or supplemental metadata. This extended metadata is generally a
 * L2 shim header and its inclusion in the packet copied to the CPU is optional.
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

#include <shr/shr_error.h>
#include <shr/shr_util.h>
#include <sal/sal_libc.h>
#include <bcmpkt/bcmpkt_rxpmd_ext.h>
#include <bcmpkt/bcmpkt_rxpmd_ext_internal.h>
#include <bcmpkt/bcmpkt_buf.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_ext_fget_t _bd##_rxpmd_ext_fget;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_ext_fget,
static const bcmpkt_rxpmd_ext_fget_t *rxpmd_ext_fget[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_ext_fset_t _bd##_rxpmd_ext_fset;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_ext_fset,
static const bcmpkt_rxpmd_ext_fset_t *rxpmd_ext_fset[] = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    extern const bcmpkt_rxpmd_ext_figet_t _bd##_rxpmd_ext_figet;
#include <bcmdrd/bcmdrd_devlist.h>

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    &_bd##_rxpmd_ext_view_info_get,
static void (*view_info_get[])(bcmpkt_pmd_view_info_t *info) = {
    NULL,
#include <bcmdrd/bcmdrd_devlist.h>
    NULL
};

static const shr_enum_map_t field_names[] =
{
    BCMPKT_RXPMD_EXT_FIELD_NAME_MAP_INIT
};

int
bcmpkt_rxpmd_ext_field_get(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd_ext,
                        int fid, uint32_t *val)
{

    if ((rxpmd_ext == NULL) || (val == NULL)) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_RXPMD_EXT_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_ext_fget[dev_type] == NULL ||
        rxpmd_ext_fget[dev_type]->fget[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    *val = rxpmd_ext_fget[dev_type]->fget[fid](rxpmd_ext);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_ext_field_set(bcmdrd_dev_type_t dev_type, uint32_t *rxpmd_ext,
                        int fid, uint32_t val)
{

    if (rxpmd_ext == NULL) {
        return SHR_E_PARAM;
    }

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }

    if (fid < 0 || fid >= BCMPKT_RXPMD_EXT_FID_COUNT) {
        return SHR_E_PARAM;
    }

    if (rxpmd_ext_fset[dev_type] == NULL ||
        rxpmd_ext_fset[dev_type]->fset[fid] == NULL) {
        return SHR_E_UNAVAIL;
    }

    rxpmd_ext_fset[dev_type]->fset[fid](rxpmd_ext, val);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_ext_field_name_get(int fid, char **name)
{

    if (name == NULL) {
        return SHR_E_PARAM;
    }

    if (fid <= BCMPKT_RXPMD_EXT_FID_INVALID ||
        fid >= BCMPKT_RXPMD_EXT_FID_COUNT) {
        return SHR_E_PARAM;
    }

    *name = field_names[fid].name;

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_ext_field_id_get(char *name, int *fid)
{
    int i;

    if ((name == NULL) || (fid == NULL)) {
        return SHR_E_PARAM;
    }

    for (i = BCMPKT_RXPMD_EXT_FID_INVALID + 1; i < BCMPKT_RXPMD_EXT_FID_COUNT; i++) {
        if (sal_strcasecmp(field_names[i].name, name) == 0) {
            *fid = field_names[i].val;
            return SHR_E_NONE;
        }
    }

    return SHR_E_NOT_FOUND;
}

int
bcmpkt_rxpmd_ext_fid_support_get(bcmdrd_dev_type_t dev_type,
                              bcmpkt_rxpmd_ext_fid_support_t *support)
{
    int i;
    bcmpkt_pmd_view_info_t view_info;

    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return SHR_E_PARAM;
    }
    if (view_info_get[dev_type] == NULL) {
        return SHR_E_INTERNAL;
    }
    if (support == NULL) {
        return SHR_E_PARAM;
    }
    sal_memset(support, 0, sizeof(*support));

    view_info_get[dev_type](&view_info);
    if ((view_info.view_types == NULL) || (view_info.view_infos == NULL)) {
        return SHR_E_UNAVAIL;
    }

    for (i = BCMPKT_RXPMD_EXT_FID_INVALID + 1; i < BCMPKT_RXPMD_EXT_FID_COUNT; i++) {
        if (view_info.view_infos[i] >= -1) {
            SHR_BITSET(support->fbits, i);
        }
    }

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_ext_get(bcmpkt_packet_t *packet, uint16_t ethertype,
                     uint32_t **data)
{
    uint8_t *data_ptr;
    uint16_t pkt_etype;

    if (packet == NULL) {
        return SHR_E_PARAM;
    }

    if (data == NULL) {
        return SHR_E_PARAM;
    }

    /* RXPMD_EXT is at offset of DST_MAC(6) + SRC_MAC(6) bytes from start of data */
    data_ptr = (uint8_t *)BCMPKT_PACKET_DATA(packet);

    /* Extract ethertype from packet at offset 12 (after DST_MAC + SRC_MAC) */
    pkt_etype = ((uint16_t)data_ptr[12] << 8) | data_ptr[13];

    /* Only get RXPMD_EXT if ethertype matches the marker */
    if (pkt_etype != ethertype) {
        return SHR_E_UNAVAIL;
    }

    *data = (uint32_t *)(data_ptr + 12);

    return SHR_E_NONE;
}

int
bcmpkt_rxpmd_ext_remove(bcmpkt_packet_t *packet, uint16_t ethertype)
{
    uint8_t *data_ptr;
    uint8_t *mac_data;
    uint8_t mac_buffer[12];
    uint16_t packet_ethertype;

    if (packet == NULL) {
        return SHR_E_PARAM;
    }

    data_ptr = (uint8_t *)BCMPKT_PACKET_DATA(packet);

    /* Extract ethertype from packet at offset 12 (after DST_MAC + SRC_MAC) */
    packet_ethertype = (uint16_t)(shr_htonl(*(uint32_t *)(data_ptr + 12)) >> 16);

    /* Only remove RXPMD_EXT if ethertype matches the provided value */
    if (packet_ethertype != ethertype) {
        return SHR_E_UNAVAIL;
    }

    /* Save MAC addresses before pulling */
    sal_memcpy(mac_buffer, data_ptr, 12);

    /* Pull RXPMD_EXT header */
    mac_data = bcmpkt_pull(packet->data_buf, BCMPKT_RXPMD_EXT_SIZE_BYTES);
    if (mac_data == NULL) {
        return SHR_E_PARAM;
    }

    /* Copy MAC addresses back to the beginning */
    sal_memcpy(mac_data, mac_buffer, 12);

    return SHR_E_NONE;
}
