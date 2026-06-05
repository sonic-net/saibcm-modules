/*! \file bcmpkt_knet.h
 *
 * KNET access interfaces, including virtual network interface (netif)
 * management and KNET packet filter.
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

#ifndef BCMPKT_KNET_H
#define BCMPKT_KNET_H

#ifdef PKTIO_IMPL
#include <pktio_dep.h>
#else
#include <sal/sal_types.h>
#include <shr/shr_types.h>
#endif
#include <bcmpkt/bcmpkt_rxpmd.h>
#include <bcmpkt/bcmpkt_flexhdr.h>
#include <bcmpkt/bcmpkt_packet.h>

/*! Maximum chars are allowed in network interface name.  */
#define BCMPKT_NETIF_NAME_MAX           16

/**
 * \name Network interface creation flags.
 * \anchor BCMPKT_NETIF_F_XXX
 */
/*! \{ */
/*!
 * RCPU header encapsulation is used for deliver packet metadata to/from
 * applications.
 * Applications should encapsulate RCPU header when send packets to the netif,
 * and may parser RCPU header when receive packets from the netif. UNET could be
 * created on such netif, which supports RCPU encapsulation, only.
 */
#define BCMPKT_NETIF_F_RCPU_ENCAP       (1 << 0)
/*!
 * Bind this netif with a RX DMA channel. All packets from
 * the DMA channel will be forwarded to this netif.
 */
#define BCMPKT_NETIF_F_BIND_RX_CH       (1 << 1)

/*!
 * Bind this network interface to a front-panel switch port for
 * transmission.  All packets from this network interface will be
 * forwarded to the bound switch port.
 */
#define BCMPKT_NETIF_F_BIND_TX_PORT     (1 << 2)

/*!
 * Add VLAN tag for packets transmitted via this network interface.
 */
#define BCMPKT_NETIF_F_ADD_TAG          (1 << 3)

/*!
 * Create network interface with specified ID.
 */
#define BCMPKT_NETIF_F_WITH_ID          (1 << 4)

/*!
 * Bingding packet metadata for transmission.
 */
#define BCMPKT_NETIF_F_BIND_TX_PMD      (1 << 5)

/*!
 * Replace network interface with ID.
 */
#define BCMPKT_NETIF_F_REPLACE          (1 << 6)

/*!
 * Use shared net device on composite devices.
 */
#define BCMPKT_NETIF_F_USE_SHARED_NDEV  (1 << 10)

/*!
 * Network interface user data size in bytes.
 */
#define BCMPKT_NETIF_USER_DATA_SIZE      64

/*! \} */

/*!
 * \brief Packet network interface structure.
 *
 * This structure defines a network interface configuration.
 *
 * "id" is allocated by \ref bcmpkt_netif_create_f function.
 *
 * "mac_addr" and "vlan" are the network interface's Layer 2 configuration.
 * "port", "port_encap" and "port_queue" are for binding a switch front port.
 *
 * "dma_chan_id" works for binding a DMA channel with a netif.
 */
typedef struct bcmpkt_netif_s {

    /*! Network interface ID.  */
    int id;

    /*! Creation flag. Refer to \ref BCMPKT_NETIF_F_XXX flags. */
    uint32_t flags;

    /*! MAC address associated with this network interface. */
    shr_mac_t mac_addr;

    /*!
     * Default VLAN ID associated with this network interface.
     * When the \ref BCMPKT_NETIF_F_ADD_TAG flag is configured, every TX packet
     * will be added a VLAN tag with this value.
     */
    uint16_t vlan;

    /*!
    * Bind with a network interface for transmit destination port. All packet
    * sent from the netif will be forwarded to this port directly.
    */
    int port;

    /*!
    * "port_encap" works with "port" to tell the encapsulation type of the port.
    * "ieee", "higig" or "higig2".
    *  Only support "ieee" currently. NULL string is taken as "ieee".
    */
    char port_encap[10];

    /*!
    * "port_queue" works with "port" to tell egress queue number.
    */
    int port_queue;

    /*!
    * This parameter works with \ref BCMPKT_NETIF_F_BIND_RX_CH to bind a RX DMA
    * channel with this netif.
    */
    int dma_chan_id;

    /*!
    * Max packet size is for this netif. Any packet which size is bigger than
    * this setting will be dropped.
    */
    uint32_t max_frame_size;

    /*! Network interface name. */
    char name[BCMPKT_NETIF_NAME_MAX];

    /*! Network interface user data. */
    uint8_t user_data[BCMPKT_NETIF_USER_DATA_SIZE];

    /*! Packet metadata for transmission. */
    bcmpkt_pmd_t tx_pmd;

} bcmpkt_netif_t;

/*!
 * \brief Network interface traverse callback function type.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif Network interface.
 * \param [in] cb_data Application-provided context for callback function.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_FAIL Failed.
 */
typedef int (*bcmpkt_netif_traverse_cb_f) (int unit,
                                           const bcmpkt_netif_t *netif,
                                           void *cb_data);

/*!
 * \brief Network interface create function.
 *
 * This function is used for creating a new network interface.
 *
 * The bcmpkt_netif_t.id is an output parameter.
 *
 * \param [in] unit Switch unit number.
 * \param [in,out] netif Network interface handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameter failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_netif_create_f) (int unit, bcmpkt_netif_t *netif);

/*!
 * \brief Network interface get function.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [out] netif Network interface handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_BADID Invalid network interface ID.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_NOT_FOUND Not found the entry.
 */
typedef int (*bcmpkt_netif_get_f) (int unit, int netif_id,
                                   bcmpkt_netif_t *netif);

/*!
 * \brief Network interface traverse function.
 *
 * This function is for traversing all network interfaces' configuration.
 * The callback function will be called for each present netif. Refer to
 * \ref bcmpkt_netif_traverse_cb_f for callback function type.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cb_func Network interface traverse callback function.
 * \param [in] cb_data Application-provided context for callback function.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_netif_traverse_f) (int unit,
                                        bcmpkt_netif_traverse_cb_f cb_func,
                                        void *cb_data);

/*!
 * \brief Network destroy function.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_BADID Invalid network interface ID.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_RESOURCE Not allowed to destroy a UNET netif.
 */
typedef int (*bcmpkt_netif_destroy_f) (int unit, int netif_id);

/*!
 * \brief Network interface link state update function.
 *
 * Notify the network stack of a change in port link state.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] linkup Link state of the port bound on the Network interface.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_BADID Invalid network interface ID.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_netif_link_update_f) (int unit, int netif_id, bool linkup);

/*!
 * \brief Get the max number of network interface.
 *
 * \param [in] unit Switch unit number.
 * \param [out] max_num Max number of network interface.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_netif_max_get_f) (int unit, int *max_num);

/**
 * \name KNET filter types.
 * \anchor BCMPKT_FILTER_T_XXX
 */
/*! \{ */
/*! Packet filter for RX direction. */
#define BCMPKT_FILTER_T_RX_PKT    1
/*! \} */

/**
 * \name KNET filter flags.
 * \anchor BCMPKT_FILTER_F_XXX
 */
/*! \{ */
/*! Match any packet. */
#define BCMPKT_FILTER_F_ANY_DATA  0x00000001

/*! Strip VLAN tag from packets sent to vitual network interfaces. */
#define BCMPKT_FILTER_F_STRIP_TAG 0x00000002

/*! Match RX DMA channel. */
#define BCMPKT_FILTER_F_MATCH_DMA_CHAN 0x00000004
/*! \} */

/**
 * \name KNET filter destination types.
 * \anchor BCMPKT_DEST_T_XXX
 */
/*! \{ */
/*! Null destination (drop packet). */
#define BCMPKT_DEST_T_NULL        0

/*! Send packet to virtual network interface. */
#define BCMPKT_DEST_T_NETIF       1

/*! Send packet to BCM Rx API. */
#define BCMPKT_DEST_T_BCM_RX_API  2

/*! Send packet to kernel filter call-back function. */
#define BCMPKT_DEST_T_CALLBACK    3
/*! \} */

/**
 * \name KNET filter match flags.
 * \anchor BCMPKT_FILTER_M_XXX
 */
/*! \{ */
/*! Match raw packet data */
#define BCMPKT_FILTER_M_RAW           0x00000001
/*! Match VLAN ID */
#define BCMPKT_FILTER_M_VLAN          0x00000002
/*! Match local ingress port */
#define BCMPKT_FILTER_M_INGPORT       0x00000004
/*! Match source module port */
#define BCMPKT_FILTER_M_SRC_MODPORT   0x00000008
/*! Match source module ID */
#define BCMPKT_FILTER_M_SRC_MODID     0x00000010
/*! Match copy-to-CPU reason code */
#define BCMPKT_FILTER_M_REASON        0x00000020
/*! Match filter processor rule number */
#define BCMPKT_FILTER_M_FP_RULE       0x00000040
/*! Match error bit */
#define BCMPKT_FILTER_M_ERROR         0x00000080
/*! Match CPU queue (rx queue) */
#define BCMPKT_FILTER_M_CPU_QUEUE     0x00000100
/*!
 * Match raw meta data. When the flag is set, the other BCMPKT_FILTER_M_XXX
 * flags are not allowed to be set together except BCMPKT_FILTER_M_RAW and
 * BCMPKT_FILTER_M_ERROR.
 */
#define BCMPKT_FILTER_M_RAW_METADATA  0x00000200
/*! \} */

/*! Filter description maximum size */
#define BCMPKT_FILTER_DESC_MAX    32

/*! Packet raw data maximum size */
#define BCMPKT_FILTER_SIZE_MAX    256

/*!
 * Filter user data size in bytes.
 */
#define BCMPKT_FILTER_USER_DATA_SIZE      64

/*! \brief Packet filter structure.
 */
typedef struct bcmpkt_filter_s {
    /*! Filter ID. */
    int id;
    /*! Filter type. Refer to \ref BCMPKT_FILTER_T_XXX. */
    uint32_t type;
    /*! Filter flags. Refer to \ref BCMPKT_FILTER_F_XXX. */
    uint32_t flags;
    /*! Filter priority (0 is highest). */
    uint32_t priority;
    /*! Destination type. Refer to \ref BCMPKT_DEST_T_XXX. */
    uint32_t dest_type;
    /*! Filter destination ID. */
    int dest_id;
    /*!
    * If non-zero this value overrides the default protocol type when
    * matching packet is passed to network stack.
    */
    uint16_t dest_proto;
    /*! Destination type. Refer to \ref BCMPKT_DEST_T_XXX. */
    uint32_t mirror_type;
    /*! Mirror destination ID. */
    int mirror_id;
    /*!
    * If non-zero this value overrides the default protocol type when
    * matching packet is passed to network stack.
    */
    uint16_t mirror_proto;

    /*!
     * Source RX DMA channel to match. \ref BCMPKT_FILTER_F_MATCH_DMA_CHAN
     * is required to take effect. Otherwise, filter won't match RX DMA
     * channel.
     */
    int dma_chan;

    /*! Refer to \ref BCMPKT_FILTER_M_XXX. */
    uint32_t match_flags;

    /*! Filter description (optional) */
    char desc[BCMPKT_FILTER_DESC_MAX];
    /*! VLAN ID to match. */
    uint16_t m_vlan;
    /*! Local ingress port to match. */
    int m_ingport;
    /*! Source module port to match. */
    int m_src_modport;
    /*! Source module ID to match. */
    int m_src_modid;
    /*! Source CPU port queue to match. */
    int m_cpu_queue;
    /*! Copy-to-CPU reason to match. */
    bcmpkt_rx_reasons_t m_reason;
    /*! Copy-to-CPU flex reason to match. */
    bcmpkt_bitmap_t m_flex_reason;
    /*! Filter processor rule to match. */
    uint32_t m_fp_rule;
    /*! Size of valid raw data and mask. */
    uint32_t raw_size;
    /*! Raw data to match. */
    uint8_t m_raw_data[BCMPKT_FILTER_SIZE_MAX];
    /*! Raw data mask for match. */
    uint8_t m_raw_mask[BCMPKT_FILTER_SIZE_MAX];
    /*! User data. */
    uint8_t user_data[BCMPKT_FILTER_USER_DATA_SIZE];
    /*! Raw metadata to match. */
    uint32_t m_raw_metadata[BCMPKT_RXPMD_SIZE_WORDS];
    /*! Raw metadata mask for match. */
    uint32_t m_raw_metadata_mask[BCMPKT_RXPMD_SIZE_WORDS];
} bcmpkt_filter_t;

/*!
 * \brief Network filter traverse callback function type.
 *
 * \param [in] unit Switch unit number.
 * \param [in] filter KNET filter handle.
 * \param [in] cb_data Application-provided context for callback function.
 *
 * \retval SHR_E_NONE No Error.
 * \retval SHR_E_FAIL Failed.
 */
typedef int (*bcmpkt_filter_traverse_cb_f) (int unit,
                                            const bcmpkt_filter_t *filter,
                                            void *cb_data);

/*!
 * \brief Network filter create function.
 *
 * This function is to be called for creating a new network filter.
 * The filter may be used for dispatching matched packets to a specific
 * destination, and/or mirror those to another destination, or drop those
 * directly.
 *
 * The bcmpkt_filter_t.id is an output parameter.
 *
 * \param [in] unit Switch unit number.
 * \param [in,out] filter KNET filter handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_filter_create_f) (int unit, bcmpkt_filter_t *filter);

/*!
 * \brief Network filter get function.
 *
 * \param [in] unit Switch unit number.
 * \param [in] filter_id KNET filter ID.
 * \param [out] filter KNET filter handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_NOT_FOUND Not found the entry.
 */
typedef int (*bcmpkt_filter_get_f) (int unit, int filter_id,
                                    bcmpkt_filter_t *filter);

/*!
 * \brief Network filter traverse function.
 *
 * This function for traversing all network filter configuration.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cb_func KNET filter traverse callback function.
 * \param [in] cb_data Application-provided context for callback function.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_filter_traverse_f) (int unit,
                                         bcmpkt_filter_traverse_cb_f cb_func,
                                         void *cb_data);

/*!
 * \brief Network filter destroy function.
 *
 *
 * \param [in] unit Switch unit number.
 * \param [in] filter_id KNET filter ID.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_filter_destroy_f) (int unit, int filter_id);

/*!
 * \brief Get the max number of filter.
 *
 * \param [in] unit Switch unit number.
 * \param [out] max_num Max number of filter.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_filter_max_get_f) (int unit, int *max_num);

/*!
 * \brief Suspend or resume TX.
 *
 * \param [in] unit Switch unit number.
 * \param [in] on True to suspend TX packets or false to resume.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_knet_tx_suspend_set_f) (int unit, bool on);

/**
 * \name OneSync PTP clock driver sub command.
 *
 * Sub commands are used to configure PHC driver and
 * OneSync FW.
 *
 * \anchor BCMPKT_ONESYNC_XXX
 */
/*! \{ */
/*! Initialize PHC driver. */
#define BCMPKT_ONESYNC_HW_INIT                      0
/*! Clean up PHC driver. */
#define BCMPKT_ONESYNC_HW_CLEANUP                   1
/*! \} */

/*! Maximum data from user. */
#define BCMPKT_ONESYNC_USER_DATA_MAX                12

/*! Maximum IEEE1588 packet metadata. */
#define BCMPKT_ONESYNC_IEEE1588_PKT_METADATA_MAX    72

/*! \brief OneSync PHC structure.
 */
typedef struct bcmpkt_onesync_s {
    /*! Device Unit. */
    int unit;
    /*! PCH Sub command. Refer BCMPKT_ONESYNC_XXX. */
    int sub_cmd;
    /*! Data specific to sub command. */
    int32_t data[BCMPKT_ONESYNC_USER_DATA_MAX];
} bcmpkt_onesync_t;

/*!
 * \brief OneSync configuration.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cfg Configuration information.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_FAIL Access driver failed.
 */
typedef int (*bcmpkt_onesync_cfg_f) (int unit, bcmpkt_onesync_t *cfg);

/*!
 * \brief Network interface operation vector.
 */
typedef struct bcmpkt_netif_ops_s {

    /*! Create a netif. */
    bcmpkt_netif_create_f create;

    /*! Get a netif's configuration. */
    bcmpkt_netif_get_f get;

    /*! Traverse netifs. */
    bcmpkt_netif_traverse_f traverse;

    /*! Destroy a netif. */
    bcmpkt_netif_destroy_f destroy;

    /*! Port link update of the netif. */
    bcmpkt_netif_link_update_f link_update;

    /*! Get max number of network interface. */
    bcmpkt_netif_max_get_f netif_max_get;
} bcmpkt_netif_ops_t;

/*!
 * \brief KNET filter operation vector.
 */
typedef struct bcmpkt_filter_ops_s {

    /*! Create a filter. */
    bcmpkt_filter_create_f create;

    /*! Get a filter configuration. */
    bcmpkt_filter_get_f get;

    /*! Traverse filter configurations. */
    bcmpkt_filter_traverse_f traverse;

    /*! Destroy a filter. */
    bcmpkt_filter_destroy_f destroy;

    /*! Get max number of filter. */
    bcmpkt_filter_max_get_f filter_max_get;
} bcmpkt_filter_ops_t;

/*!
 * \brief KNET vector.
 */
typedef struct bcmpkt_knet_s {
    /*! initialized flag: 0 - uninitialized 1 - initialized. */
    int initialized;

    /*! KNET driver name, such as "KNET". */
    char driver_name[128];

     /*! Netif operations vector. */
    bcmpkt_netif_ops_t netif_ops;

    /*! KNET filter operations vector. */
    bcmpkt_filter_ops_t filter_ops;

    /*! KNET TX suspend. */
    bcmpkt_knet_tx_suspend_set_f tx_suspend_set;

    /*! Broadcom OneSync configuration. */
    bcmpkt_onesync_cfg_f onesync_cfg;

} bcmpkt_knet_t;

/*!
 * \brief Register KNET operations.
 *
 * Enable KNET operations.
 *
 * \param [in] knet_drv KNET vectors.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_CONFIG Invalid driver.
 * \retval SHR_E_PARAM Check parameters failed.
 */
extern int
bcmpkt_knet_drv_register(bcmpkt_knet_t *knet_drv);

/*!
 * \brief Unregister KNET driver.
 *
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_BUSY The driver is in using.
 */
extern int
bcmpkt_knet_drv_unregister(void);

/*!
 * \brief Network interface create function.
 *
 * This function is used for creating a new network interface.
 *
 * The bcmpkt_netif_t.id is an output parameter.
 *
 * \param [in] unit Switch unit number.
 * \param [in,out] netif Network interface handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameter failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_netif_create(int unit, bcmpkt_netif_t *netif);

/*!
 * \brief Network interface get function.
 *
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [out] netif Network interface handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_BADID Invalid network interface ID.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_NOT_FOUND Not found the entry.
 */
extern int
bcmpkt_netif_get(int unit, int netif_id, bcmpkt_netif_t *netif);

/*!
 * \brief Network interface traverse function.
 *
 * This function is for traversing all network interfaces' configuration.
 * The callback function will be called for each present netif. Refer to
 * \ref bcmpkt_netif_traverse_cb_f for callback function type.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cb_func Network interface traverse callback function.
 * \param [in] cb_data Application-provided context for callback function.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_netif_traverse(int unit, bcmpkt_netif_traverse_cb_f cb_func,
                      void *cb_data);

/*!
 * \brief Network destroy function.
 *
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_BADID Invalid network interface ID.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_RESOURCE Not allowed to destroy a UNET netif.
 */
extern int
bcmpkt_netif_destroy(int unit, int netif_id);

/*!
 * \brief Network interface link state update function.
 *
 * Notify the network stack of a change in port link state.
 *
 * \param [in] unit Switch unit number.
 * \param [in] netif_id Network interface ID.
 * \param [in] linkup Link state of the port bound on the Network interface.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_BADID Invalid network interface ID.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_netif_link_update(int unit, int netif_id, bool linkup);

/*!
 * \brief Get the max number of network interface.
 *
 * \param [in] unit Switch unit number.
 * \param [out] max_num Max number of network interface.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_netif_max_get(int unit, int *max_num);

/*!
 * \brief Network filter create function.
 *
 * This function is to be called for creating a new network filter.
 * The filter may be used for dispatching matched packets to a specific
 * destination, and/or mirror those to another destination, or drop those
 * directly.
 *
 * The bcmpkt_filter_t.id is an output parameter.
 *
 * \param [in] unit Switch unit number.
 * \param [in,out] filter KNET filter handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_filter_create(int unit, bcmpkt_filter_t *filter);

/*!
 * \brief Network filter get function.
 *
 *
 * \param [in] unit Switch unit number.
 * \param [in] filter_id KNET filter ID.
 * \param [out] filter KNET filter handle.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_NOT_FOUND Not found the entry.
 */
extern int
bcmpkt_filter_get(int unit, int filter_id, bcmpkt_filter_t *filter);

/*!
 * \brief Network filter traverse function.
 *
 * This function for traversing all network filter configuration.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cb_func KNET filter traverse callback function.
 * \param [in] cb_data Application-provided context for callback function.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_PARAM Check parameters failed.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_filter_traverse(int unit, bcmpkt_filter_traverse_cb_f cb_func,
                       void *cb_data);

/*!
 * \brief Network filter destroy function.
 *
 *
 * \param [in] unit Switch unit number.
 * \param [in] filter_id KNET filter ID.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_filter_destroy(int unit, int filter_id);

/*!
 * \brief Get the max number of filter.
 *
 * \param [in] unit Switch unit number.
 * \param [out] max_num Max number of filter.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit number.
 * \retval SHR_E_CONFIG Network device was not initialized.
 * \retval SHR_E_FAIL Access driver failed.
 */
extern int
bcmpkt_filter_max_get(int unit, int *max_num);

/*!
 * \brief PHC initialize function.
 *
 * This function is to be called for initilizing the PTP clock driver.
 * The PHC releated sub command in dispatched from KNET driver.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cfg sub command information.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_RESOURCE FW is not initialized.
 */
extern int
bcmpkt_onesync_init(int unit, bcmpkt_onesync_t *cfg);

/*!
 * \brief PHC clean up function.
 *
 * This function is to be called for deinitilizing the PTP clock driver.
 * The PHC releated sub command in dispatched from KNET driver.
 *
 * \param [in] unit Switch unit number.
 * \param [in] cfg sub command information.
 *
 * \retval SHR_E_NONE Success.
 * \retval SHR_E_UNIT Invalid unit.
 * \retval SHR_E_FAIL Access driver failed.
 * \retval SHR_E_CONFIG Network device was not initialized.
 */
extern int
bcmpkt_onesync_cleanup(int unit, bcmpkt_onesync_t *cfg);

#endif /* BCMPKT_KNET_H */
