/*! \file ngknet_parser.c
 *
 * Parser routines for Knet RX.
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

#include "ngknet_parser.h"
#include "ngknet_buff.h"

static int debug = 0;

/* DCB words */
#define BKN_DNX_DCB_WORDS           4
/* 0x1 - Jericho 2 mode */
#define BKN_DNX_JR2_MODE            1
/* PTCH_2 */
#define BKN_DNX_PTCH_2_SIZE         2
/* ITMH */
#define BKN_DNX_ITMH_SIZE           5
/* Modlue Header */
#define BKN_DNX_MODULE_HEADER_SIZE   16
/* FTMH */
#define BKN_DNX_FTMH_TC_MSB                            14
#define BKN_DNX_FTMH_TC_NOF_BITS                       3
#define BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_MSB        17
#define BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_NOF_BITS   16
#define BKN_DNX_FTMH_PP_DSP_MSB                        33
#define BKN_DNX_FTMH_PP_DSP_NOF_BITS                   8
#define BKN_DNX_FTMH_ACTION_TYPE_MSB                   43
#define BKN_DNX_FTMH_ACTION_TYPE_NOF_BITS              2
#define BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_MSB            73
#define BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_NOF_BITS       1
#define BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_MSB            74
#define BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_NOF_BITS       1
#define BKN_DNX_FTMH_TM_DST_EXT_PRESENT_MSB            75
#define BKN_DNX_FTMH_TM_DST_EXT_PRESENT_NOF_BITS       1
#define BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_MSB         76
#define BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_NOF_BITS    1
#define BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_MSB              77
#define BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_NOF_BITS         1
#define BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_MSB             78
#define BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_NOF_BITS        1
/* Fix Length for FTMH and Extension headers */
#define BKN_DNX_FTMH_BASE_SIZE                         10
#define BKN_DNX_FTMH_BIER_BFR_EXT_SIZE                 2
#define BKN_DNX_FTMH_TM_DST_EXT_SIZE                   3
#define BKN_DNX_FTMH_FLOW_ID_EXT_SIZE                  3
#define BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE             6
#define BKN_DNX_SYSPORTS_PER_DEVICE_MASK               0x3FF
/*ASE*/
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_MSB              0
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_NOF_BITS         4
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_1588          2
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_NTP           3
#define BKN_DNX_FTMH_ASE_TYPE_MSB                      47
#define BKN_DNX_FTMH_ASE_TYPE_NOF_BITS                 1
#define BKN_DNX_FTMH_ASE_TYPE_OAM                      0
/* TSH */
#define BKN_DNX_TSH_SIZE                               4
/* PPH */
#define BKN_DNX_INTERNAL_BASE_TYPE_9                        9
#define BKN_DNX_INTERNAL_BASE_TYPE_10                       10
#define BKN_DNX_INTERNAL_BASE_TYPE_12                       12
#define BKN_DNX_INTERNAL_9_FORWARD_DOMAIN_MSB               5
#define BKN_DNX_INTERNAL_9_FORWARD_DOMAIN_NOF_BITS          16
#define BKN_DNX_INTERNAL_9_LEARN_EXT_PRESENT_MSB            53
#define BKN_DNX_INTERNAL_9_LEARN_EXT_PRESENT_NOF_BITS       1
#define BKN_DNX_INTERNAL_9_FHEI_SIZE_MSB                    54
#define BKN_DNX_INTERNAL_9_FHEI_SIZE_NOF_BITS               2
#define BKN_DNX_INTERNAL_9_LIF_EXT_TYPE_MSB                 56
#define BKN_DNX_INTERNAL_9_LIF_EXT_TYPE_NOF_BITS            3
#define BKN_DNX_INTERNAL_10_FORWARD_DOMAIN_MSB              9
#define BKN_DNX_INTERNAL_10_FORWARD_DOMAIN_NOF_BITS         16
#define BKN_DNX_INTERNAL_10_LEARN_EXT_PRESENT_MSB           61
#define BKN_DNX_INTERNAL_10_LEARN_EXT_PRESENT_NOF_BITS      1
#define BKN_DNX_INTERNAL_10_FHEI_SIZE_MSB                   62
#define BKN_DNX_INTERNAL_10_FHEI_SIZE_NOF_BITS              2
#define BKN_DNX_INTERNAL_10_LIF_EXT_TYPE_MSB                64
#define BKN_DNX_INTERNAL_10_LIF_EXT_TYPE_NOF_BITS           3
#define BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_MSB              21
#define BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_NOF_BITS         18
#define BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_MSB           77
#define BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_NOF_BITS      1
#define BKN_DNX_INTERNAL_12_FHEI_SIZE_MSB                   78
#define BKN_DNX_INTERNAL_12_FHEI_SIZE_NOF_BITS              2
#define BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_MSB                80
#define BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_NOF_BITS           3
#define BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_MSB                83
#define BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_NOF_BITS           7
/* PPH.FHEI_TYPE */
#define BKN_DNX_INTERNAL_FHEI_TYPE_SZ0                      1
#define BKN_DNX_INTERNAL_FHEI_TYPE_SZ1                      2
#define BKN_DNX_INTERNAL_FHEI_TYPE_SZ2                      3
/* FHEI */
#define BKN_DNX_INTERNAL_FHEI_SZ0_SIZE                      3
#define BKN_DNX_INTERNAL_FHEI_SZ1_SIZE                      5
#define BKN_DNX_INTERNAL_FHEI_SZ2_SIZE                      8
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_MSB         0
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_NOF_BITS    27
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_MSB              27
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_NOF_BITS         9
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_MSB              36
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_NOF_BITS         4
/* PPH Extension */
#define BKN_DNX_INTERNAL_LEARN_EXT_SIZE                     19
/* UDH */
#define BKN_DNX_UDH_DATA_TYPE_0_MSB                    0
#define BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS               2
#define BKN_DNX_UDH_DATA_TYPE_1_MSB                    2
#define BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS               2
#define BKN_DNX_UDH_DATA_TYPE_2_MSB                    4
#define BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS               2
#define BKN_DNX_UDH_DATA_TYPE_3_MSB                    6
#define BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS               2
#define BKN_DNX_UDH_BASE_SIZE                          1
/* TOD SECOND header */
#define BKN_DNX_TOD_SECOND_SIZE                        4
/* OIBIH */
#define BKN_DNX_OIBIH_SIZE          14
#define BKN_DNX_OIBIH_OAM_PDU_OFFSET_MSB               104
#define BKN_DNX_OIBIH_OAM_PDU_OFFSET_NOF_BITS          8


/* PPH fwd_domain type. */
#define BKN_DNX_PPH_FWD_DOMAIN_TYPE_VSI                0
#define BKN_DNX_PPH_FWD_DOMAIN_TYPE_VRF                3

#define BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(_fwd_domain) (((_fwd_domain) >> 16) & 0x3)
#define BKN_DNX_PPH_FWD_DOMAIN_ID_GET(_fwd_domain)   ((_fwd_domain) & 0xffff)
#define BKN_DNX_PPH_FWD_DOMAIN_IS_VSI(_fwd_domain)   (BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(_fwd_domain) == BKN_DNX_PPH_FWD_DOMAIN_TYPE_VSI)
#define BKN_DNX_PPH_FWD_DOMAIN_IS_VRF(_fwd_domain)   (BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(_fwd_domain) == BKN_DNX_PPH_FWD_DOMAIN_TYPE_VRF)

#define BKN_DNX_INGRESS_TRAP_ID_TRAP_OAM_LEVEL       (162)
#define BKN_DNX_INGRESS_TRAP_ID_TRAP_OAM_PASSIVE     (172)

#define BKN_DNX_SPA_MODE_16_BITS                      (0)
#define BKN_DNX_SPA_MODE_17_BITS                      (1)
#define BKN_DNX_SPA_MODE_18_BITS                      (2)

/* ftmh action type. */
typedef enum bkn_dpp_ftmh_action_type_e {
    BKN_DPP_FTMH_ACTION_TYPE_FORWARD = 0, /* TM action is forward */
    BKN_DPP_FTMH_ACTION_TYPE_SNOOP = 1,   /* TM action is snoop */
    BKN_DPP_FTMH_ACTION_TYPE_INBOUND_MIRROR = 2, /* TM action is inbound mirror. */
    BKN_DPP_FTMH_ACTION_TYPE_OUTBOUND_MIRROR = 3 /* TM action is outbound mirror. */
}bkn_dpp_ftmh_action_type_t;

/* ftmh dest extension. */
typedef struct bkn_dpp_ftmh_dest_extension_s {
    uint8_t valid; /* Set if the extension is present */
    uint32_t dst_sys_port; /* Destination System Port */
} bkn_dpp_ftmh_dest_extension_t;

/* dnx packet */
typedef struct bkn_dune_system_header_info_s {
    uint32_t system_header_size;
    struct {
        uint32_t tc;                          /* traffic class */
        uint32_t action_type;                 /* Indicates if the copy is one of the Forward Snoop or Mirror packet copies */
        uint32_t source_sys_port_aggregate;   /* Source System port*/
    } ftmh;
    struct {
        uint32_t parsing_start_offset;
        uint32_t forward_domain;
        uint32_t trap_qualifier;
        uint32_t trap_id;
    } internal;
    /** Flags for RX header parser */
    /** Indicates whether 1st system header is following */
#define BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER      (0x1 << 0)
    /** Indicates whether 2nd system header is following */
#define BKN_RX_HEADER_F_HAS_TWO_SYSTEM_HEADER      (0x1 << 1)
    /** Indicates whether TSH is following */
#define BKN_RX_HEADER_F_HAS_TSH                    (0x1 << 2)
    /** Indicates whether OTSH is following */
#define BKN_RX_HEADER_F_HAS_OTSH                   (0x1 << 3)
    /** Indicates whether internal header is following */
#define BKN_RX_HEADER_F_HAS_INTERNAL_HEADER        (0x1 << 4)
    /** Indicates whether TOD second header is following */
#define BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND      (0x1 << 5)
    uint32_t flags;
} bkn_dune_system_header_info_t;

#define BKN_DNX_BIT(x) (1<<(x))
#define BKN_DNX_RBIT(x) (~(1<<(x)))
#ifdef __LITTLE_ENDIAN
#define BKN_DNX_BYTE_SWAP(x) (x)
#else
#define BKN_DNX_BYTE_SWAP(x) ((((x) << 24)) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) >> 24)))
#endif

static void
bkn_bitstream_set_field(uint32_t *input_buffer, uint32_t start_bit, uint32_t  nof_bits, uint32_t field)
{
    uint32_t place;
    uint32_t field_bit_i;
    uint32_t bit_indicator;

    if( nof_bits > 32)
    {
        return;
    }

    for( place=start_bit, field_bit_i = 0; field_bit_i< nof_bits; ++place, ++field_bit_i)
    {
        bit_indicator = field & BKN_DNX_BIT(nof_bits-field_bit_i-1);
        if(bit_indicator)
        {
            input_buffer[place>>5] |= (0x80000000 >> (place & 0x0000001F));
        }
        else
        {
            input_buffer[place>>5] &= ~(0x80000000 >> (place & 0x0000001F));
        }
    }
    return;
}

static void
bkn_bitstream_get_field(uint8_t  *input_buffer, uint32_t start_bit, uint32_t  nof_bits, uint32_t *output_value)
{
    uint32_t idx;
    uint32_t buf_sizes=0;
    uint32_t tmp_output_value[2]={0};
    uint32_t first_byte_ndx;
    uint32_t last_byte_ndx;
    uint32_t place;
    uint32_t field_bit_i;
    uint8_t *tmp_output_value_u8_ptr = (uint8_t*)&tmp_output_value;
    uint32_t bit_indicator;

    if (nof_bits > 32)
    {
         return;
    }

    first_byte_ndx = start_bit / 8;
    last_byte_ndx = ((start_bit + nof_bits - 1) / 8);
    *output_value=0;

    /* get 32 bit value, MSB */
    for (idx = first_byte_ndx; idx <= last_byte_ndx; ++idx)
    {
        tmp_output_value_u8_ptr[last_byte_ndx - idx] = input_buffer[idx];
        buf_sizes += 8;
    }
    tmp_output_value[0] = BKN_DNX_BYTE_SWAP(tmp_output_value[0]);
    if (last_byte_ndx > 4)
    {
       tmp_output_value[1] = BKN_DNX_BYTE_SWAP(tmp_output_value[1]);
    }

    place = buf_sizes - (start_bit % 8 + nof_bits);
    for (field_bit_i = 0; field_bit_i< nof_bits; ++place, ++field_bit_i)
    {
        uint32_t result;
        result = tmp_output_value[place>>5] & BKN_DNX_BIT(place & 0x0000001F);
        if (result)
        {
           bit_indicator = 1;
        } else {
           bit_indicator = 0;
        }
        *output_value |=  bit_indicator << field_bit_i;
    }
    return;
}

static int
bkn_dnx_packet_parse_ftmh(
    struct adapter_dev *adev,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint32_t fld_val;
    uint32_t pkt_offset = packet_info->system_header_size;
    uint8_t  tm_dst_ext_present = 0;
    uint8_t  app_specific_ext_size = 0;
    uint8_t  flow_id_ext_size = 0;
    uint8_t  bier_bfr_ext_size = 0;

    if ((adev == NULL) || (buf == NULL) || (packet_info == NULL)) {
        return -1;
    }

    /* FTMH: Traffic-Class */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_TC_MSB,
            BKN_DNX_FTMH_TC_NOF_BITS,
            &fld_val);
    packet_info->ftmh.tc = fld_val;
    /* FTMH: Source-System-Port-Aggregate */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_MSB,
            BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_NOF_BITS,
            &fld_val);
    packet_info->ftmh.source_sys_port_aggregate = fld_val;
    /* FTMH: Action-Type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_ACTION_TYPE_MSB,
            BKN_DNX_FTMH_ACTION_TYPE_NOF_BITS,
            &fld_val);
    packet_info->ftmh.action_type = fld_val;
    /* FTMH: PPH-Type TSH */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_MSB,
            BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_NOF_BITS,
            &fld_val);
    packet_info->flags |= fld_val ? BKN_RX_HEADER_F_HAS_TSH : 0;
    /* FTMH: PPH-Type PPH base */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_MSB,
            BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_NOF_BITS,
            &fld_val);
    packet_info->flags |= fld_val ? BKN_RX_HEADER_F_HAS_INTERNAL_HEADER : 0;
    /* FTMH: TM-Destination-Extension-Present */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_TM_DST_EXT_PRESENT_MSB,
            BKN_DNX_FTMH_TM_DST_EXT_PRESENT_NOF_BITS,
            &fld_val);
    tm_dst_ext_present = fld_val;
    /* FTMH: Application-Specific-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_MSB,
            BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_NOF_BITS,
            &fld_val);
    app_specific_ext_size = fld_val;
    /* FTMH: Flow-ID-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_MSB,
            BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_NOF_BITS,
            &fld_val);
    flow_id_ext_size = fld_val;
    /* FTMH: BIER-BFR-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_MSB,
            BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_NOF_BITS,
            &fld_val);
    bier_bfr_ext_size = fld_val;

    pkt_offset += BKN_DNX_FTMH_BASE_SIZE;

    DBG_VERB(("FTMH(10-%u): traffic-class %u source-system-port 0x%x action_type %u\n",
              pkt_offset, packet_info->ftmh.tc, packet_info->ftmh.source_sys_port_aggregate,
              packet_info->ftmh.action_type));

    /* FTMH LB-Key Extension */
    if (adev->rsi.ftmh_lb_key_size > 0)
    {
        pkt_offset += adev->rsi.ftmh_lb_key_size;
        DBG_VERB(("FTMH LB-Key Extension(%u-%u) is present\n", adev->rsi.ftmh_lb_key_size, pkt_offset));
    }
    /* FTMH Stacking Extension */
    if (adev->rsi.ftmh_stacking_ext_size > 0)
    {
        pkt_offset += adev->rsi.ftmh_stacking_ext_size;
        DBG_VERB(("FTMH Stacking Extension(%u-%u) is present\n", adev->rsi.ftmh_stacking_ext_size, pkt_offset));
    }
    if (adev->rsi.spa_mode == BKN_DNX_SPA_MODE_16_BITS)
    {
        /* FTMH BIER BFR Extension */
        if (bier_bfr_ext_size > 0)
        {
            pkt_offset += BKN_DNX_FTMH_BIER_BFR_EXT_SIZE;
            DBG_VERB(("FTMH BIER BFR Extension(2-%u) is present\n", pkt_offset));
        }
    }
    if (adev->rsi.spa_mode == BKN_DNX_SPA_MODE_16_BITS)
    {
        /* FTMH TM Destination Extension */
        if (tm_dst_ext_present > 0)
        {
            pkt_offset += BKN_DNX_FTMH_TM_DST_EXT_SIZE;
            DBG_VERB(("FTMH TM Destination Extension(3-%u) is present\n", pkt_offset));
        }
    }
    if (adev->rsi.spa_mode == BKN_DNX_SPA_MODE_18_BITS)
    {
        packet_info->ftmh.source_sys_port_aggregate |= (tm_dst_ext_present ? 1:0) << 16;
        packet_info->ftmh.source_sys_port_aggregate |= (bier_bfr_ext_size  ? 1:0) << 17;
        DBG_VERB(("FTMH(10): source-system-port(18b) 0x%x\n", packet_info->ftmh.source_sys_port_aggregate));
    }
    /* FTMH Application Specific Extension */
    if (app_specific_ext_size > 0)
    {
        bkn_bitstream_get_field(
                &buf[pkt_offset],
                BKN_DNX_FTMH_ASE_TYPE_MSB,
                BKN_DNX_FTMH_ASE_TYPE_NOF_BITS,
                &fld_val);
        if (fld_val == BKN_DNX_FTMH_ASE_TYPE_OAM) {
            /* ASE: OAM_SUB_TYPE */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_MSB,
                    BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_NOF_BITS,
                    &fld_val);
            if ((fld_val == BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_1588) ||
                (fld_val == BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_NTP)) {
                packet_info->flags |= BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND;
            }
        }
        pkt_offset += BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE;
        DBG_VERB(("FTMH Application Specific Extension(6-%u) is present\n", pkt_offset));
    }
    /* FTMH Flow-ID Extension */
    if (flow_id_ext_size > 0)
    {
        pkt_offset += BKN_DNX_FTMH_FLOW_ID_EXT_SIZE;
        DBG_VERB(("FTMH Flow-ID Extension(3-%u) is present\n", pkt_offset));
    }
    DBG_VERB(("FTMH flags = 0x%08x\n", packet_info->flags));

    packet_info->system_header_size = pkt_offset;

    return 0;
}


static int
bkn_dnx_packet_parse_internal(
    struct adapter_dev *adev,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info,
    uint8_t is_oamp_punted,
    uint8_t *is_trapped)
{
    uint32_t fld_val;
    uint32_t pkt_offset = packet_info->system_header_size;
    uint8_t  learn_ext_present;
    uint8_t  fhei_size;
    uint8_t  lif_ext_type;
    uint8_t  udh_en = adev->rsi.udh_enabled;

    if ((adev == NULL) || (buf == NULL) || (packet_info == NULL)) {
        return -1;
    }

    /* Internal: Forward-Domain */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_MSB,
            BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_NOF_BITS,
            &fld_val);
    packet_info->internal.forward_domain = fld_val;
    /* Internal: Learn-Extension-Present */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_MSB,
            BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_NOF_BITS,
            &fld_val);
    learn_ext_present = fld_val;
    /* Internal: FHEI-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_FHEI_SIZE_MSB,
            BKN_DNX_INTERNAL_12_FHEI_SIZE_NOF_BITS,
            &fld_val);
    fhei_size = fld_val;
    /* Internal: LIF-Extension-Type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_MSB,
            BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_NOF_BITS,
            &fld_val);
    lif_ext_type = fld_val;

    /* Internal: Parsing-Start-Offset */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_MSB,
            BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_NOF_BITS,
            &fld_val);
    packet_info->internal.parsing_start_offset = fld_val;

    pkt_offset += BKN_DNX_INTERNAL_BASE_TYPE_12;
    DBG_VERB(("Internal(12-%u): FWD_DOMAIN 0x%x(%d,%d), LEARN_EXT %d, FHEI_SIZE %d, LIF_EXT %d \n",
                pkt_offset, packet_info->internal.forward_domain,
                BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(packet_info->internal.forward_domain),
                BKN_DNX_PPH_FWD_DOMAIN_ID_GET(packet_info->internal.forward_domain),
                learn_ext_present, fhei_size, lif_ext_type));

    if (fhei_size)
    {
        switch (fhei_size)
        {
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ0:
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ0_SIZE;
                DBG_VERB(("FHEI(3-%u) is present\n", pkt_offset));
                break;
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ1:
                /* FHEI: Type */
                bkn_bitstream_get_field(
                        &buf[pkt_offset],
                        BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_MSB,
                        BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_NOF_BITS,
                        &fld_val);
                /* FHEI-Size == 5B, FHEI-Type == Trap/Sniff */
                if (fld_val == 0x5)
                {
                    /* Action_Type: 0-Forward, 1-Snoop, 2-Mirror, 3-StatisticalSampling */
                    if (!packet_info->ftmh.action_type) {
                        *is_trapped = TRUE;
                    }
                    /* FHEI: Qualifier */
                    bkn_bitstream_get_field(
                            &buf[pkt_offset],
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_MSB,
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_NOF_BITS,
                            &fld_val);
                    packet_info->internal.trap_qualifier = fld_val;
                    /* FHEI: Code */
                    bkn_bitstream_get_field(
                            &buf[pkt_offset],
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_MSB,
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_NOF_BITS,
                            &fld_val);
                    packet_info->internal.trap_id= fld_val;
                }
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ1_SIZE;
                DBG_VERB(("FHEI(5-%u): code 0x%x qualifier 0x%x\n", pkt_offset, packet_info->internal.trap_id, packet_info->internal.trap_qualifier));
                break;
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ2:
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ2_SIZE;
                DBG_VERB(("FHEI(8-%u) is present\n", pkt_offset));
                break;
        }
    }

    /* PPH LIF Extension */
    if (lif_ext_type)
    {
        pkt_offset += adev->rsi.pph_lif_ext_size[lif_ext_type];
        DBG_VERB(("PPH LIF Extension(%d-%u) is present\n", adev->rsi.pph_lif_ext_size[lif_ext_type], pkt_offset));
    }

    /* PPH Learn Extension */
    if (learn_ext_present)
    {
        pkt_offset += BKN_DNX_INTERNAL_LEARN_EXT_SIZE;
        DBG_VERB(("PPH Learn Extension(19-%u) is present\n", pkt_offset));
    }

    /** Skip UDH If packet is punted to CPU by OAMP */
    if (is_oamp_punted) {
        udh_en = FALSE;
    }

    /* OAM DMM/DMR TOD second header: PPH+TOD+UDH */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER) {
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND) {
            pkt_offset += BKN_DNX_TOD_SECOND_SIZE;
            if (packet_info->internal.parsing_start_offset >= BKN_DNX_TOD_SECOND_SIZE) {
                packet_info->internal.parsing_start_offset -= BKN_DNX_TOD_SECOND_SIZE;
            }
            DBG_VERB(("TOD second Header(4-%u) is present\n", pkt_offset));
        }
    }

    /* UDH Header */
    if (udh_en)
    {
        uint8_t data_type_0;
        uint8_t data_type_1;
        uint8_t data_type_2;
        uint8_t data_type_3;

        if (ADAPTER_IS_CMICR(adev))
        {
            /* UDH: UDH-Data-Type[3] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_0_MSB,
                    BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS,
                    &fld_val);
            data_type_3 = fld_val;
            /* UDH: UDH-Data-Type[2] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_1_MSB,
                    BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS,
                    &fld_val);
            data_type_2 = fld_val;
            /* UDH: UDH-Data-Type[1] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_2_MSB,
                    BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS,
                    &fld_val);
            data_type_1 = fld_val;
            /* UDH: UDH-Data-Type[0] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_3_MSB,
                    BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS,
                    &fld_val);
            data_type_0 = fld_val;
            pkt_offset += BKN_DNX_UDH_BASE_SIZE;

            if (data_type_0)
            {
                pkt_offset += adev->rsi.udh_length_type[0];
            }
            if (data_type_1)
            {
                pkt_offset += adev->rsi.udh_length_type[1];
            }
            if (data_type_2)
            {
                pkt_offset += adev->rsi.udh_length_type[2];
            }
            if (data_type_3)
            {
                pkt_offset += adev->rsi.udh_length_type[3];
            }
        }
        else
        {
            /* UDH: UDH-Data-Type[0] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_0_MSB,
                    BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS,
                    &fld_val);
            data_type_0 = fld_val;
            /* UDH: UDH-Data-Type[1] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_1_MSB,
                    BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS,
                    &fld_val);
            data_type_1 = fld_val;
            /* UDH: UDH-Data-Type[2] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_2_MSB,
                    BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS,
                    &fld_val);
            data_type_2 = fld_val;
            /* UDH: UDH-Data-Type[3] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_3_MSB,
                    BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS,
                    &fld_val);
            data_type_3 = fld_val;
            pkt_offset += BKN_DNX_UDH_BASE_SIZE;

            pkt_offset += adev->rsi.udh_length_type[data_type_0];
            pkt_offset += adev->rsi.udh_length_type[data_type_1];
            pkt_offset += adev->rsi.udh_length_type[data_type_2];
            pkt_offset += adev->rsi.udh_length_type[data_type_3];
        }
        DBG_VERB(("UDH base(1-%u) is present\n", pkt_offset));
    }

    packet_info->system_header_size = pkt_offset;

    return 0;
}

static int
bkn_dnx_packet_parse_header(
    struct adapter_dev *adev,
    uint8_t *buff,
    uint32_t buff_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint8_t  is_oamp_punted = FALSE;
    uint8_t  is_trapped = FALSE;
    uint8_t  idx = 0;
    uint8_t  is_2_system_header_from_trap = FALSE;
    uint32_t cpu_trap_qualifier;

    if ((adev == NULL) || (buff == NULL) || (packet_info == NULL)) {
        return -1;
    }

    packet_info->flags = BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER;
    /* FTMH */
    bkn_dnx_packet_parse_ftmh(adev, buff, buff_len, packet_info);

    /* Time-Stamp */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_TSH)
    {
        packet_info->system_header_size += BKN_DNX_TSH_SIZE;
        DBG_VERB(("Time-Stamp Header(4-%u) is present\n", packet_info->system_header_size));
    }

    /* Check if packet was punted to CPU by OAMP */
    for (idx = 0; idx < adev->rsi.oamp_port_number; idx++)
    {
        if (packet_info->ftmh.source_sys_port_aggregate == adev->rsi.oamp_ports[idx])
        {
            is_oamp_punted = TRUE;
            break;
        }
    }

    /* Internal */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
    {
        bkn_dnx_packet_parse_internal(adev, buff, buff_len, packet_info, is_oamp_punted, &is_trapped);
    }

    cpu_trap_qualifier = packet_info->internal.trap_qualifier & 0xffff;

    if (is_trapped && cpu_trap_qualifier == 0)
    {
        /*
         * For egress trap such as oam up mep destination 1, oam level error
         * and down mep passive, the trapped packet might have 2 sets of system header
         */
        switch (packet_info->internal.trap_id)
        {
            case BKN_DNX_INGRESS_TRAP_ID_TRAP_OAM_LEVEL:
            case BKN_DNX_INGRESS_TRAP_ID_TRAP_OAM_PASSIVE:
                is_2_system_header_from_trap = TRUE;
                break;
            default:
                /** Get ingress cpu trap id of oam up mep destination 1. */
                if (adev->rsi.up_mep_ingress_cpu_trap_id1 == packet_info->internal.trap_id)
                {
                    is_2_system_header_from_trap = TRUE;
                    break;
                }
                if (adev->rsi.system_headers_mode == BKN_DNX_JR2_MODE)
                {
                    /** Get ingress cpu trap id of oam up mep destination 2. */
                    if (adev->rsi.up_mep_ingress_cpu_trap_id2 == packet_info->internal.trap_id)
                    {
                        is_2_system_header_from_trap = TRUE;
                        break;
                    }
                }
        }
    }

    if ((is_oamp_punted && is_trapped) || is_2_system_header_from_trap)
    {
        uint32_t oibih_oam_pdu_offset = 0;
        is_trapped = FALSE;

        packet_info->flags = BKN_RX_HEADER_F_HAS_TWO_SYSTEM_HEADER;
        if (ADAPTER_IS_CMICR(adev))
        {
            if (is_oamp_punted)
            {
                /* OIBIH: OAM_PDU_Offset */
                bkn_bitstream_get_field(
                        &buff[packet_info->system_header_size],
                        BKN_DNX_OIBIH_OAM_PDU_OFFSET_MSB,
                        BKN_DNX_OIBIH_OAM_PDU_OFFSET_NOF_BITS,
                        &oibih_oam_pdu_offset);
                packet_info->system_header_size += BKN_DNX_OIBIH_SIZE;
                DBG_VERB(("OIBIH Header(14-%u) is present\n", packet_info->system_header_size));
            }
        }
        /* FTMH */
        bkn_dnx_packet_parse_ftmh(adev, buff, buff_len, packet_info);
        /* Time-Stamp */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_TSH)
        {
            packet_info->system_header_size += BKN_DNX_TSH_SIZE;
            DBG_VERB(("Time-Stamp Header(4-%u) is present\n", packet_info->system_header_size));
        }
        /* Internal */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
        {
            bkn_dnx_packet_parse_internal(adev, buff, buff_len, packet_info, FALSE, &is_trapped);
        }
        if (oibih_oam_pdu_offset)
        {
            /*
             * parsing_start_offset indicates the bytes before OAM PDU including PTCH, etc. For example, it's the length of PTCH + ETH1
             * oibih_oam_pdu_offset indicates the bytes from the end of system headers to OAM PDU. For example, it's the length of ETH1
             */
            if (packet_info->internal.parsing_start_offset > oibih_oam_pdu_offset)
            {
                packet_info->system_header_size += (packet_info->internal.parsing_start_offset - oibih_oam_pdu_offset);
                DBG_VERB(("Offset after system headers %u\n", (packet_info->internal.parsing_start_offset - oibih_oam_pdu_offset)));
            }
        }
    }
    else
    {
        /*
         * J2,J3 devices does not have PTCH header. NO need to calculate parsing_start_offset.
         * The future devices will have PTCH header. Should consider how to calculate the Eth
         * header position.
         */
        if (packet_info->internal.parsing_start_offset && ADAPTER_IS_CMICR(adev))
        {
            packet_info->system_header_size += packet_info->internal.parsing_start_offset;
            DBG_VERB(("Offset after system headers %u\n", packet_info->internal.parsing_start_offset));
        }
    }

    DBG_VERB(("Total length of headers is %u (0x%x)\n", packet_info->system_header_size, packet_info->system_header_size));

    return 0;
}

static int
bkn_dnx_packet_parse_ai_ftmh(
    struct adapter_dev *adev,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint32_t fld_val;
    uint32_t pkt_offset = packet_info->system_header_size;
    uint8_t  app_specific_ext_size = 0;

    if ((adev == NULL) || (buf == NULL) || (packet_info == NULL)) {
        return -1;
    }

    /* FTMH: Traffic-Class */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_TC_MSB,
            BKN_DNX_FTMH_TC_NOF_BITS,
            &fld_val);
    packet_info->ftmh.tc = fld_val;
    /* FTMH: Source-System-Port-Aggregate */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_MSB,
            BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_NOF_BITS,
            &fld_val);
    packet_info->ftmh.source_sys_port_aggregate = fld_val;
    /* FTMH: Action-Type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_ACTION_TYPE_MSB,
            BKN_DNX_FTMH_ACTION_TYPE_NOF_BITS,
            &fld_val);
    packet_info->ftmh.action_type = fld_val;
    /* FTMH: PPH-Type TSH */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_MSB,
            BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_NOF_BITS,
            &fld_val);
    packet_info->flags |= fld_val ? BKN_RX_HEADER_F_HAS_TSH : 0;
    /* FTMH: PPH-Type PPH base */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_MSB,
            BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_NOF_BITS,
            &fld_val);
    packet_info->flags |= fld_val ? BKN_RX_HEADER_F_HAS_INTERNAL_HEADER : 0;
    /* FTMH: Application-Specific-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_MSB,
            BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_NOF_BITS,
            &fld_val);
    app_specific_ext_size = fld_val;

    pkt_offset += BKN_DNX_FTMH_BASE_SIZE;

    DBG_VERB(("FTMH(10-%u): traffic-class %u source-system-port 0x%x action_type %u\n",
              pkt_offset, packet_info->ftmh.tc, packet_info->ftmh.source_sys_port_aggregate,
              packet_info->ftmh.action_type));

    /* FTMH LB-Key Extension */
    if (adev->rsi.ftmh_lb_key_size > 0)
    {
        pkt_offset += adev->rsi.ftmh_lb_key_size;
        DBG_VERB(("FTMH LB-Key Extension(%u-%u) is present\n", adev->rsi.ftmh_lb_key_size, pkt_offset));
    }
    /* FTMH Application Specific Extension */
    if (app_specific_ext_size > 0)
    {
        pkt_offset += BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE;
        DBG_VERB(("FTMH Application Specific Extension(6-%u) is present\n", pkt_offset));
    }

    DBG_VERB(("FTMH flags = 0x%08x\n", packet_info->flags));

    packet_info->system_header_size = pkt_offset;

    return 0;
}


static int
bkn_dnx_packet_parse_ai_internal(
    struct adapter_dev *adev,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info,
    uint8_t is_oamp_punted,
    uint8_t *is_trapped)
{
    uint32_t fld_val;
    uint32_t pkt_offset = packet_info->system_header_size;
    uint8_t  fhei_size;
    uint8_t  udh_en = adev->rsi.udh_enabled;

    if ((adev == NULL) || (buf == NULL) || (packet_info == NULL)) {
        return -1;
    }
    /* Internal: FHEI-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_FHEI_SIZE_MSB,
            BKN_DNX_INTERNAL_12_FHEI_SIZE_NOF_BITS,
            &fld_val);
    fhei_size = fld_val;

    /* Internal: PPH base */
    pkt_offset += BKN_DNX_INTERNAL_BASE_TYPE_12;
    DBG_VERB(("Internal(12-%u): FHEI_SIZE %d \n", pkt_offset, fhei_size));

    if (fhei_size)
    {
        switch (fhei_size)
        {
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ0:
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ0_SIZE;
                DBG_VERB(("FHEI(3-%u) is present\n", pkt_offset));
                break;
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ1:
                /* FHEI: Type */
                bkn_bitstream_get_field(
                        &buf[pkt_offset],
                        BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_MSB,
                        BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_NOF_BITS,
                        &fld_val);
                /* FHEI-Size == 5B, FHEI-Type == Trap/Sniff */
                if (fld_val == 0x5)
                {
                    /* Action_Type: 0-Forward, 1-Snoop, 2-Mirror, 3-StatisticalSampling */
                    if (!packet_info->ftmh.action_type) {
                        *is_trapped = TRUE;
                    }
                    /* FHEI: Qualifier */
                    bkn_bitstream_get_field(
                            &buf[pkt_offset],
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_MSB,
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_NOF_BITS,
                            &fld_val);
                    packet_info->internal.trap_qualifier = fld_val;
                    /* FHEI: Code */
                    bkn_bitstream_get_field(
                            &buf[pkt_offset],
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_MSB,
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_NOF_BITS,
                            &fld_val);
                    packet_info->internal.trap_id= fld_val;
                }
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ1_SIZE;
                DBG_VERB(("FHEI(5-%u): code 0x%x qualifier 0x%x\n", pkt_offset, packet_info->internal.trap_id, packet_info->internal.trap_qualifier));
                break;
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ2:
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ2_SIZE;
                DBG_VERB(("FHEI(8-%u) is present\n", pkt_offset));
                break;
        }
    }

    /* UDH Header */
    if (udh_en)
    {
        uint8_t data_type_0;
        uint8_t data_type_1;
        uint8_t data_type_2;
        uint8_t data_type_3;

        if (ADAPTER_IS_CMICR(adev))
        {
            /* UDH: UDH-Data-Type[3] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_0_MSB,
                    BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS,
                    &fld_val);
            data_type_3 = fld_val;
            /* UDH: UDH-Data-Type[2] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_1_MSB,
                    BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS,
                    &fld_val);
            data_type_2 = fld_val;
            /* UDH: UDH-Data-Type[1] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_2_MSB,
                    BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS,
                    &fld_val);
            data_type_1 = fld_val;
            /* UDH: UDH-Data-Type[0] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_3_MSB,
                    BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS,
                    &fld_val);
            data_type_0 = fld_val;
            pkt_offset += BKN_DNX_UDH_BASE_SIZE;

            if (data_type_0)
            {
                pkt_offset += adev->rsi.udh_length_type[0];
            }
            if (data_type_1)
            {
                pkt_offset += adev->rsi.udh_length_type[1];
            }
            if (data_type_2)
            {
                pkt_offset += adev->rsi.udh_length_type[2];
            }
            if (data_type_3)
            {
                pkt_offset += adev->rsi.udh_length_type[3];
            }
        }
        DBG_VERB(("UDH base(1-%u) is present\n", pkt_offset));
    }

    packet_info->system_header_size = pkt_offset;

    return 0;
}

static int
bkn_dnx_packet_parse_ai_header(
    struct adapter_dev *adev,
    uint8_t *buff,
    uint32_t buff_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint8_t  is_oamp_punted = FALSE;
    uint8_t  is_trapped = FALSE;

    if ((adev == NULL) || (buff == NULL) || (packet_info == NULL)) {
        return -1;
    }

    packet_info->flags = BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER;
    /* FTMH */
    bkn_dnx_packet_parse_ai_ftmh(adev, buff, buff_len, packet_info);

    /* Time-Stamp */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_TSH)
    {
        packet_info->system_header_size += BKN_DNX_TSH_SIZE;
        DBG_VERB(("Time-Stamp Header(4-%u) is present\n", packet_info->system_header_size));
    }

    /* Internal */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
    {
        bkn_dnx_packet_parse_ai_internal(adev, buff, buff_len, packet_info, is_oamp_punted, &is_trapped);
    }

    DBG_VERB(("Total length of headers is %u (0x%x)\n", packet_info->system_header_size, packet_info->system_header_size));

    return 0;
}

static void
bkn_dnx_scratch_data_generate(
    struct adapter_dev *adev,
    struct sk_buff *skb,
    bkn_dune_system_header_info_t *packet_info,
    int queue_id,
    uint32_t *sand_scratch_data)
{
    struct pdma_dev *pdev = adev->pdma_dev;
    struct dev_ctrl *ctrl = &pdev->ctrl;
    struct queue_group *grp = NULL;
    struct pdma_rx_queue *rxq = NULL;
    uint32_t *ring = NULL, *dcb = NULL;
    struct pdma_rx_buf *pbuf = NULL;
    int gi, qi;
    bkn_bitstream_set_field(sand_scratch_data, 0,  16,
                            packet_info->internal.trap_id);
    bkn_bitstream_set_field(sand_scratch_data, 16, 16,
                            packet_info->internal.trap_qualifier);
    bkn_bitstream_set_field(sand_scratch_data, 32, 18,
                            packet_info->ftmh.source_sys_port_aggregate);
    bkn_bitstream_set_field(sand_scratch_data, 64, 2,
                            packet_info->ftmh.action_type);
    bkn_bitstream_set_field(sand_scratch_data, 66, 18,
                            packet_info->internal.forward_domain);

    for (gi = 0; gi < pdev->num_groups; gi++) {
        grp = &ctrl->grp[gi];
        if (!grp->attached) {
            continue;
        }
        for (qi = 0; qi < pdev->grp_queues; qi++) {
            if (1 << qi & grp->bm_rxq) {
                rxq = grp->rx_queue[qi];
                if (queue_id == rxq->queue_id) {
                    break;
                }
            }
        }
        if (qi == pdev->grp_queues) {
            continue;
        }
        ring = rxq->ring;
        pbuf = &rxq->pbuf[rxq->curr];
        if (pbuf->skb == skb) {
            dcb = &ring[rxq->curr * BKN_DNX_DCB_WORDS];
            break;
        }
    }
    if (dcb) {
        DBG_VERB(("DCB(gi %d, qi %d, curr %d): {%08x,%08x,%08x,%08x}\n", gi, qi, rxq->curr, dcb[0],dcb[1],dcb[2],dcb[3]));
        sand_scratch_data[BKN_DNX_DCB_WORDS - 1] = dcb[BKN_DNX_DCB_WORDS - 1];
    }
}

static int
ngknet_packet_header_parse(
    struct adapter_dev *adev,
    struct sk_buff *skb)
{
    bkn_dune_system_header_info_t packet_info = {0};
    struct pkt_buf *pkb = (struct pkt_buf *)skb->data;
    struct pkt_hdr *pkh = &pkb->pkh;
    uint32_t sand_scratch_data[NGKNET_SCRATCH_DATA_WORDS] = {0};
    void *ssdp = NULL;

    /* Jericho 2 mode */
    if (ADAPTER_IS_AI(adev)) {
        bkn_dnx_packet_parse_ai_header(adev, &pkb->data, pkh->meta_len + pkh->data_len, &packet_info);
    } else {
        bkn_dnx_packet_parse_header(adev, &pkb->data, pkh->meta_len + pkh->data_len, &packet_info);
    }
    pkh->meta_len += packet_info.system_header_size;
    pkh->data_len -= packet_info.system_header_size;
    bkn_dnx_scratch_data_generate(adev, skb, &packet_info, pkh->queue_id, sand_scratch_data);
    ssdp = skb_put(skb, sizeof(sand_scratch_data));
    memcpy(ssdp, sand_scratch_data, sizeof(sand_scratch_data));

    return 0;
}

int
ngknet_rx_parser_debug_set(
    int debug_lvl)
{
    debug = debug_lvl;
    return 0;
}

int
ngknet_rx_parser(
    struct adapter_dev *adev,
    struct sk_buff *skb)
{
    return ngknet_packet_header_parse(adev, skb);
}

bool
ngknet_rx_parser_scratch_data_match(
     struct adapter_dev *adev,
     struct sk_buff *skb,
     ngknet_filter_t *filt)
{
    ngknet_msg_scratch_data_t *scratch;
    uint32_t *scratch_data = NULL;
    uint32_t sdata;
    int idx;

    scratch = &adev->scratch[filt->id];
    scratch_data = (uint32_t *)&skb->data[skb->len - NGKNET_SCRATCH_DATA_BYTES];
    DBG_VERB(("Filter: %d\n", scratch->filter_id));
    for (idx = 0; idx < NGKNET_SCRATCH_DATA_WORDS; idx++) {
        DBG_VERB(("OOB[%d]: 0x%08x [0x%08x]\n", idx, scratch->data[idx], scratch->mask[idx]));
    }
    DBG_VERB(("Meta Data [+ Selected Raw packet data]\n"));
    for (idx = 0; idx < NGKNET_SCRATCH_DATA_WORDS; idx++) {
        DBG_VERB(("Scratch[%d]: 0x%08x\n", idx, scratch_data[idx]));
    }
    for (idx = 0; idx < NGKNET_SCRATCH_DATA_WORDS; idx++) {
        sdata = scratch_data[idx] & scratch->mask[idx];
        if (sdata != scratch->data[idx]) {
            return false;
        }
    }
    return true;
}
