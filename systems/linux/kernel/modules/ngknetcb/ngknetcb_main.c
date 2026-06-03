/*! \file ngknetcb_main.c
 *
 * NGKNET Callback module entry.
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

#include <lkm/lkm.h>
#include <lkm/ngknet_kapi.h>
#ifdef KPMD
#include <bcmpkt/bcmpkt_flexhdr_internal.h>
#include <bcmpkt/bcmpkt_rxpmd_fid.h>
#include <bcmpkt/bcmpkt_flexhdr_field.h>
#include <bcmpkt/bcmpkt_lbhdr_field.h>
#include <bcmpkt/bcmpkt_rxpmd_field.h>
#include <bcmpkt/bcmpkt_txpmd_field.h>
#include <bcmpkt/bcmpkt_rxpmd_match_id.h>
#endif /* KPMD */

/*! \cond */
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("NGKNET Callback Module");
MODULE_LICENSE("GPL");
/*! \endcond */

/*! Module information */
#define NGKNETCB_MODULE_NAME    "linux_ngknetcb"

#ifndef KPMD
#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    BCMDRD_DEV_T_##_bd,
/*! Enumeration for all base device types. */
typedef enum {
    BCMDRD_DEV_T_NONE = 0,
/*! \cond */
#include <bcmdrd/bcmdrd_devlist.h>
/*! \endcond */
    BCMDRD_DEV_T_COUNT
} bcmdrd_dev_type_t;

/*! Create enumeration values from list of supported variants. */
#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
    BCMLRD_VARIANT_T_##_bd##_##_ve,

/*! Enumeration for all device variants. */
typedef enum bcmlrd_variant_e {
    BCMLRD_VARIANT_T_NONE = 0,
/*! \cond */
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
/*! \endcond */
     BCMLRD_VARIANT_T_COUNT
} bcmlrd_variant_t;
#endif /* !KPMD */

typedef struct ngknetcb_dev_s {
    bool initialized;
    bcmdrd_dev_type_t dev_type;
    bcmlrd_variant_t var_type;
} ngknetcb_dev_t;

typedef struct ngknetcb_pmd_unit_s {
    bool added; /* User cares about this pmd info */
    bool valid; /* Unit is initialized and added */
} ngknetcb_pmd_unit_t;

static ngknetcb_dev_t cb_dev[NUM_PDMA_DEV_MAX];
static ngknetcb_pmd_unit_t pmd_units[NUM_PDMA_DEV_MAX];

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    {#_bd, BCMDRD_DEV_T_##_bd},
static const struct {
    char *name;
    bcmdrd_dev_type_t dev;
} device_types[] = {
    {"device_none", BCMDRD_DEV_T_NONE},
#include <bcmdrd/bcmdrd_devlist.h>
    {"device_count", BCMDRD_DEV_T_COUNT}
};

#define BCMDRD_DEVLIST_ENTRY(_nm,_vn,_dv,_rv,_md,_pi,_bd,_bc,_fn,_cn,_pf,_pd,_r0,_r1) \
    {BCMDRD_DEV_T_##_bd, _dv},
static const struct {
    bcmdrd_dev_type_t dev_type;
    uint32 dev_id;
} device_ids[] = {
    {BCMDRD_DEV_T_NONE, 0},
#include <bcmdrd/bcmdrd_devlist.h>
    {BCMDRD_DEV_T_COUNT, 0}
};

#define BCMLRD_VARIANT_ENTRY(_bd,_bu,_va,_ve,_vu,_vv,_vo,_vd,_r0,_r1)\
    {#_bd, #_ve, BCMLRD_VARIANT_T_##_bd##_##_ve},
static const struct {
    char *dev_name;
    char *var_name;
    bcmlrd_variant_t var;
} variant_types[] = {
    {"device_none", "variant_none", BCMLRD_VARIANT_T_NONE},
#include <bcmlrd/chip/bcmlrd_chip_variant.h>
    {"device_count", "variant_count", BCMLRD_VARIANT_T_COUNT}
};

#ifdef KPMD

/* BEGIN EXAMPLE PMD CODE */

struct name_value_pair_s {
    char *name;
    int value;
};

static struct name_value_pair_s rxpmd_info[] = {
    BCMPKT_RXPMD_FIELD_NAME_MAP_INIT
};

static struct name_value_pair_s dnx_rxpmd_info[] = {
    BCMPKT_RXPMD_DNX_FIELD_NAME_MAP_INIT
};

static int
device_type_is_sand(bcmdrd_dev_type_t dev_type)
{
    uint32 dev_family;
    if (dev_type <= BCMDRD_DEV_T_NONE ||
        dev_type >= BCMDRD_DEV_T_COUNT) {
        return 0;
    }
    dev_family = device_ids[dev_type].dev_id & 0xf000;
    /**
     * 0xb000 : 56xxx (XGS)
     * 0xf000 : 78xxx (XGS)
     * 0x8000 : 88xxx (DNX)
     * 0x9000 : 99xxx (DNX)
     */
    if (dev_family == 0x8000 || dev_family == 0x9000) {
        return 1;
    }
    return 0;
}

static void
print_supported_rxpmd_fields(bcmdrd_dev_type_t dev_type)
{
    int field_id;
    bcmpkt_rxpmd_fid_support_t support;

    printk("\nRX Metadata types for this device\n");
    bcmpkt_rxpmd_fid_support_get(dev_type, &support);
    BCMPKT_RXPMD_FID_SUPPORT_ITER(support, field_id) {
        if (device_type_is_sand(dev_type)) {
            printk("Field: %2d [%s]\n", field_id, dnx_rxpmd_info[field_id].name);
        } else {
            printk("Field: %2d [%s]\n", field_id, rxpmd_info[field_id].name);
        }
    }
}

static void
print_all_rxpmd_fields(bcmdrd_dev_type_t dev_type, const uint8_t *rxpmd)
{
    int field_id;
    bcmpkt_rxpmd_fid_support_t support;
    uint32_t val;

    printk("\nRX Metadata for this packet\n");
    bcmpkt_rxpmd_fid_support_get(dev_type, &support);

    BCMPKT_RXPMD_FID_SUPPORT_ITER(support, field_id) {
        int rv = bcmpkt_rxpmd_field_get(dev_type, (uint32_t *) rxpmd, field_id,
                                        &val);
        if (rv == 0) {
            if (device_type_is_sand(dev_type)) {
                printk("    %-26s = %4d [%X]\n", dnx_rxpmd_info[field_id].name, val,
                       val);
            } else {
                printk("    %-26s = %4d [%X]\n", rxpmd_info[field_id].name, val,
                       val);
            }
        }
    }
}

static void
print_packet_tag_type(bcmdrd_dev_type_t dev_type, const uint8_t *rxpmd)
{
    bcmpkt_rxpmd_fid_support_t support;
    uint32_t val;
    const char *tag_type[4] = {
        "Untagged",
        "Inner Tagged",
        "Outer Tagged",
        "Double Tagged"
    };

    if (device_type_is_sand(dev_type)) {
        printk("\nRX Ingress Tag Type: not support on DNX family!\n");
        return;
    }

    bcmpkt_rxpmd_fid_support_get(dev_type, &support);

    if (BCMPKT_RXPMD_FID_SUPPORT_GET(support, BCMPKT_RXPMD_ING_TAG_TYPE)) {
        int             rv =
          bcmpkt_rxpmd_field_get(dev_type, (uint32_t *) rxpmd,
                                 BCMPKT_RXPMD_ING_TAG_TYPE, &val);
        if (rv == 0) {
            printk("\nRX Ingress Tag Type: %s\n", tag_type[val]);
        }
    }
}

static void
print_all_supported(bcmdrd_dev_type_t dev_type)
{
    print_supported_rxpmd_fields(dev_type);
}

/* END EXAMPLE PMD CODE */
#endif /* KPMD */

/*!
 * \brief Device Initialization Callback.
 *
 * The device initialization callback allows an external module to
 * perform device-specific initialization in preparation for Tx and Rx
 * packet processing.
 *
 * \param [in] dinfo Device information.
 *
 */
static void
init_cb(ngknet_dev_info_t *dinfo)
{
    int unit;
    bcmdrd_dev_type_t dt;
    bcmlrd_variant_t var;

    unit = dinfo->dev_no;

    if ((unsigned int)unit >= NUM_PDMA_DEV_MAX) {
        return;
    }

    for (dt = 0; dt < BCMDRD_DEV_T_COUNT; dt++) {
        if (!strcasecmp(dinfo->type_str, device_types[dt].name)) {
            cb_dev[unit].dev_type = dt;
            break;
        }
    }

    for (var = 0; var < BCMLRD_VARIANT_T_COUNT; var++) {
        if ((!strcasecmp(dinfo->type_str, variant_types[var].dev_name)) &&
            (!strcasecmp(dinfo->var_str, variant_types[var].var_name))) {
            cb_dev[unit].var_type = var;
            break;
        }
    }

    printk("%s; unit: %d; dev %s; dev_id: 0x%x variant: %s\n",
           __func__, dinfo->dev_no, dinfo->type_str, dinfo->dev_id, dinfo->var_str);
    printk("  dev_type: %d\n", cb_dev[unit].dev_type);
    printk("  variant: %d\n", cb_dev[unit].var_type);

    cb_dev[unit].initialized = true;
    if (pmd_units[unit].added == true) {
        pmd_units[unit].valid = true;
        printk("%s; setting valid for unit: %d\n", __func__, unit);
    }
#ifdef KPMD
    print_all_supported(dt);
#else
    printk("%s: Not compiled with KPMD\n", __func__);
#endif /* KPMD */
}

/* Print 16 hex bytes per line to console */
static void
print_buffer(const uint8_t *pmd, const int len)
{
    const char         *const to_hex = "0123456789ABCDEF";
    int                 i;
    char                buffer[64];
    char               *buffer_ptr;
    int                 addr = 0;

    buffer_ptr = buffer;
    for (i = 0; i < len; i++) {
        *buffer_ptr++ = ' ';
    if ((i % 16) == 8) {
            *buffer_ptr++ = ' ';
    }
        *buffer_ptr++ = to_hex[(pmd[i] >> 4) & 0xF];
        *buffer_ptr++ = to_hex[pmd[i] & 0xF];
        if (((i % 16) == 15) || (i == len - 1)) {
            *buffer_ptr = '\0';
            buffer_ptr = buffer;
            printk("%04X  %s\n", addr, buffer);
            addr = i + 1;
        }
    }
}

static void
print_mac(const uint8_t *pkt)
{
    printk("DMAC=%02X:%02X:%02X:%02X:%02X:%02X\n",
           pkt[0], pkt[1], pkt[2], pkt[3], pkt[4], pkt[5]);
}

/*!
 * \brief Rx Callback.
 *
 * The Rx call-back allows an external module to modify packet contents
 * before it is handed off to the Linux network stack.
 *
 * \param [in] skb NGKNET callback description.
 *
 */
static struct sk_buff *
test_rx_cb(struct sk_buff *skb)
{
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    int unit = cbd->dinfo->dev_no;

    if (cb_dev[unit].initialized) {
        printk("********************************\n");
        printk("%s; device %d: %s; variant: %s\n", __func__, cbd->dinfo->dev_no,
               cbd->dinfo->type_str,
               variant_types[cb_dev[unit].var_type].var_name);
        printk("  dev_type: %d\n", cb_dev[unit].dev_type);
        printk("  variant: %d\n", cb_dev[unit].var_type);

        if (cbd->pmd_len != 0) {
            printk("  Rx PMD: %d bytes\n", cbd->pmd_len);
            print_buffer(cbd->pmd, cbd->pmd_len);
#ifdef KPMD
            print_all_rxpmd_fields(cb_dev[unit].dev_type, cbd->pmd);
            print_packet_tag_type(cb_dev[unit].dev_type, cbd->pmd);
#endif /* KPMD */
            printk("  Rx Packet: %d bytes\n", cbd->pkt_len);
            print_buffer(cbd->pmd + cbd->pmd_len, cbd->pkt_len);

#ifdef KPMD
            /*
             * Use the valid flag to control pmd parsing code.
             * This is set specifying the units module parameter
             */
            if (pmd_units[unit].valid) {
                int             rv = 0;
                uint32_t        val;
                uint32_t        rxpmd_flex[32];
                uint32_t        flexdata_len_words;
                uint32_t        flexdata_len_bytes;
                uint32_t       *flexdata_addr;

                rv = bcmpkt_rxpmd_field_get(cb_dev[unit].dev_type,
                                            (uint32_t *) cbd->pmd, 1, &val);
                if (rv == 0) {
                    printk("%s; bcmpkt_rxpmd_field_get val: %d\n", __func__,
                           val);
                } else {
                    printk("%s; bcmpkt_rxpmd_field_get FAILED, rv = %d\n",
                           __func__, rv);
                }

                /* Check whether device supports flex metadata. */
                rv =
                  bcmpkt_rxpmd_flexdata_get(cb_dev[unit].dev_type, rxpmd_flex,
                                            &flexdata_addr,
                                            &flexdata_len_words);
                if (rv == 0) {
                    flexdata_len_bytes = flexdata_len_words * 4;
                    printk("%s; flexdata_len_bytes = %d\n",
                           __func__, flexdata_len_bytes);
                } else {
                    printk("%s; bcmpkt_rxpmd_flexdata_get failed, rv = %d\n",
                           __func__, rv);
                }
            }
#endif /* KPMD */
        }
        printk("%s; netif user data: 0x%08x\n", __func__,
               *(uint32_t *) cbd->netif->user_data);
        if (cbd->filt) {
            printk("%s; filter user data: 0x%08x\n", __func__,
                   *(uint32_t *) cbd->filt->user_data);
        }
    }

    return skb;
}

/*!
 * \brief Tx Callback.
 *
 * The Tx call-back allows an external module to modify packet contents
 * before it is injected into the switch.
 *
 * \param [in] skb NGKNET callback description.
 *
 */
static struct sk_buff *
test_tx_cb(struct sk_buff *skb)
{
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    int unit = cbd->dinfo->dev_no;

    if (cb_dev[unit].initialized) {
        printk("********************************\n");
        printk("%s; dev %d: %s; variant: %s\n", __func__, cbd->dinfo->dev_no,
               cbd->dinfo->type_str, variant_types[cb_dev[unit].var_type].var_name);
        printk("  dev_type: %d\n", cb_dev[unit].dev_type);
        printk("  variant: %d\n", cb_dev[unit].var_type);

        if (cbd->pmd_len != 0) {
#ifdef KPMD
            int rv = 0;
            uint32_t val;
            rv = bcmpkt_txpmd_field_get(cb_dev[unit].dev_type,
                (uint32_t *) cbd->pmd, 1, &val);
            if (rv == 0) {
                printk("%s; bcmpkt_txpmd_field_get val: %d\n", __func__, val);
            } else {
                printk("%s; bcmpkt_txpmd_field_get failed, rv = %d\n", __func__, rv);
            }
#endif /* KPMD */
            printk("  Tx PMD: %d bytes\n", cbd->pmd_len);
            print_buffer(cbd->pmd, cbd->pmd_len);
            printk("  Tx Packet: %d bytes\n", cbd->pkt_len);
            print_buffer(cbd->pmd + cbd->pmd_len, cbd->pkt_len);
        }
    }

    return skb;
}

static struct sk_buff *
test_filter_cb(struct sk_buff *skb, ngknet_filter_t **filt)
{
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);

    printk("********************************\n");
    printk("%s; dev %d: %s\n", __func__,
        cbd->dinfo->dev_no, cbd->dinfo->type_str);

    printk("  cbd->pmd_len: %d; cbd->pkt_len: %d;\n", cbd->pmd_len, cbd->pkt_len);
    if (cbd->pmd_len != 0) {
        print_buffer(cbd->pmd, cbd->pmd_len);
        print_mac(cbd->pmd + cbd->pmd_len);
    }
    if (cbd->filt) {
        printk("%s; filter user data: 0x%08x;\n", __func__,
        *(uint32_t *)cbd->filt->user_data);
    }
    if (filt) {
      *filt = NULL;
    }
    return skb;
}

static struct sk_buff *
test_a_filter_cb(struct sk_buff *skb, ngknet_filter_t **filt)
{
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    printk("%s; dev %d: %s\n", __func__,
        cbd->dinfo->dev_no, cbd->dinfo->type_str);

    printk("  cbd->pmd_len: %d; cbd->pkt_len: %d;\n", cbd->pmd_len, cbd->pkt_len);
    if (cbd->pmd_len != 0) {
        print_buffer(cbd->pmd, cbd->pmd_len);
        print_mac(cbd->pmd + cbd->pmd_len);
    }
    if (cbd->filt) {
        printk("%s; filter user data: 0x%08x\n", __func__,
               *(uint32_t *)cbd->filt->user_data);
    }
    if (filt) {
      *filt = NULL;
    }
    return skb;
}

static struct sk_buff *
test_b_filter_cb(struct sk_buff *skb, ngknet_filter_t **filt)
{
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);

    printk("%s; dev %d: %s\n", __func__,
           cbd->dinfo->dev_no, cbd->dinfo->type_str);

    printk("  cbd->pmd_len: %d; cbd->pkt_len: %d;\n", cbd->pmd_len, cbd->pkt_len);
    if (cbd->pmd_len != 0) {
        print_buffer(cbd->pmd, cbd->pmd_len);
        print_mac(cbd->pmd + cbd->pmd_len);
    }
    if (cbd->filt) {
        printk("%s; filter user data: 0x%08x;\n", __func__,
        *(uint32_t *)cbd->filt->user_data);
    }
    if (filt) {
      *filt = NULL;
    }
    return skb;
}

static char *units = NULL;
module_param(units, charp, 0660);

/*
 * Parse the units module parameter and update the pmd_units table
 *   units='0,1,2'
 *   units=0
 */
static void
pmd_parse_init(void)
{
    char *p, *s = NULL;
    int   pmd_unit;

    if (units != NULL) {
        printk("%s; unit param: %s\n", __func__, units);
    } else {
        printk("%s; unit param not provided\n", __func__);
        return;
    }
    /* Parse the unit value */
    if (strchr(units, ',') != NULL) {
        s = units;
        p = strchr(s, ',');
        while (p) {
            *p = '\0';
            pmd_unit = simple_strtol(s, &s, 10);
            printk("%s; parameter parsed for unit: %d\n", __func__, pmd_unit);
            if ((pmd_unit >= 0) && (pmd_unit < NUM_PDMA_DEV_MAX)) {
                pmd_units[pmd_unit].added = true;
                printk("%s; setting pmd_units added to true for unit: %d\n",
                __func__, pmd_unit);
            }
            s = &p[1];
            p = strchr(s, ',');
        }
        pmd_unit = simple_strtol(s, &s, 10);
        printk("%s; parameter parsed for unit: %d\n", __func__, pmd_unit);
        if ((pmd_unit >= 0) && (pmd_unit < NUM_PDMA_DEV_MAX)) {
            pmd_units[pmd_unit].added = true;
                printk("%s; setting pmd_units added to true for unit: %d\n",
            __func__, pmd_unit);
        }
    } else {
        pmd_unit = simple_strtol(units, &units, 10);
        if ((pmd_unit >= 0) && (pmd_unit < NUM_PDMA_DEV_MAX)) {
            pmd_units[pmd_unit].added = true;
                printk("%s; setting pmd_units added to true for unit: %d\n",
            __func__, pmd_unit);
        }
    }
}

static int
test_a_netif_create_cb(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif)
{
    int retv = 0;

    printk("%s; device %d: %s\n", __func__, dinfo->dev_no, dinfo->type_str);

    return retv;
}

static int
test_b_netif_create_cb(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif)
{
    int retv = 0;

    printk("%s; device %d: %s\n", __func__, dinfo->dev_no, dinfo->type_str);

    return retv;
}


static int
test_a_netif_destroy_cb(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif)
{
    int retv = 0;

    printk("%s; device %d: %s\n", __func__,
        dinfo->dev_no, dinfo->type_str);

    return retv;
}

static int
test_b_netif_destroy_cb(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif)
{
    int retv = 0;

    printk("%s; device %d: %s\n", __func__,
        dinfo->dev_no, dinfo->type_str);

    return retv;
}

static int __init
ngknetcb_init_module(void)
{
    memset(pmd_units, 0, sizeof(ngknetcb_pmd_unit_t)*NUM_PDMA_DEV_MAX);
    pmd_parse_init();

    printk("%s: Initialize and register callbacks\n", __func__);
    ngknet_rx_cb_register(test_rx_cb);
    ngknet_tx_cb_register(test_tx_cb);
    ngknet_filter_cb_register(test_filter_cb);
    ngknet_filter_cb_register_by_name(test_a_filter_cb, "test_a_filter_cb");
    ngknet_filter_cb_register_by_name(test_b_filter_cb, "test_b_filter_cb");
    ngknet_dev_init_cb_register(init_cb);

    /* register callbacks of netif create/destroy */
    ngknet_netif_create_cb_register(test_a_netif_create_cb);
    ngknet_netif_create_cb_register(test_b_netif_create_cb);
    ngknet_netif_destroy_cb_register(test_a_netif_destroy_cb);
    ngknet_netif_destroy_cb_register(test_b_netif_destroy_cb);

    return 0;
}

static void __exit
ngknetcb_exit_module(void)
{
    printk("%s: Unregister callbacks\n", __func__);
    ngknet_rx_cb_unregister(test_rx_cb);
    ngknet_tx_cb_unregister(test_tx_cb);
    ngknet_filter_cb_unregister(test_a_filter_cb);
    ngknet_filter_cb_unregister(test_b_filter_cb);
    ngknet_filter_cb_unregister(test_filter_cb);
    ngknet_dev_init_cb_unregister(init_cb);

    /* unregister callbacks of netif create/destroy */
    ngknet_netif_create_cb_unregister(test_a_netif_create_cb);
    ngknet_netif_create_cb_unregister(test_b_netif_create_cb);
    ngknet_netif_destroy_cb_unregister(test_a_netif_destroy_cb);
    ngknet_netif_destroy_cb_unregister(test_b_netif_destroy_cb);
}

module_init(ngknetcb_init_module);
module_exit(ngknetcb_exit_module);
