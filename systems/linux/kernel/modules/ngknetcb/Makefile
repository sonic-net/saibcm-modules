#
# $Copyright: 2017-2026 Broadcom Inc. All rights reserved.
# 
# Permission is granted to use, copy, modify and/or distribute this
# software under either one of the licenses below.
# 
# License Option 1: GPL
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation (the "GPL").
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License version 2 (GPLv2) for more details.
# 
# You should have received a copy of the GNU General Public License
# version 2 (GPLv2) along with this source code.
# 
# 
# License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
# 
# This software is governed by the Broadcom Open Network Switch APIs license:
# https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
# 
# 
#
# Linux NGKNET Callback module.
#

LOCALDIR = systems/linux/kernel/modules/ngknetcb
PKTIO_IMPL = 1
include ${SDK}/make/Make.config

# Include PMD library by default
ifneq (0,$(KPMD))

# Kernel module source directory
KMODDIR = $(CURDIR)

# Avoid creating links in original kernel module source directory
GENDIR = $(KMODDIR)/generated

ifneq ($(SDK_OUTDIR),)
GENDIR = $(SDK_OUTDIR)/$(SDKBUILD)/ngknetcb/generated
endif

knetcb: kpmd
	$(MAKE) -C $(GENDIR) all

#
# Redefine KAPI interface API name to avoid symbol conflict.
#
KAPI_FUNCS = ngknet_dev_init_cb_register \
             ngknet_dev_init_cb_unregister \
             ngknet_rx_cb_register \
             ngknet_rx_cb_unregister \
             ngknet_tx_cb_register \
             ngknet_tx_cb_unregister \
             ngknet_netif_create_cb_register \
             ngknet_netif_create_cb_unregister \
             ngknet_netif_destroy_cb_register \
             ngknet_netif_destroy_cb_unregister \
             ngknet_filter_cb_register \
             ngknet_filter_cb_register_by_name \
             ngknet_filter_cb_unregister \
             bcmpkt_rxpmd_field_list_dump \
             bcmpkt_rxpmd_dump \
             bcmpkt_rxpmd_len_get \
             bcmpkt_rxpmd_field_get \
             bcmpkt_rxpmd_field_set \
             bcmpkt_rxpmd_mh_get \
             bcmpkt_rxpmd_flexdata_get \
             bcmpkt_rxpmd_reasons_get \
             bcmpkt_rxpmd_reasons_set \
             bcmpkt_rxpmd_field_name_get \
             bcmpkt_rxpmd_field_id_get \
             bcmpkt_rxpmd_fid_support_get      \
             bcmpkt_txpmd_field_list_dump \
             bcmpkt_txpmd_dump \
             bcmpkt_txpmd_dnx_field_list_dump \
             bcmpkt_txpmd_dnx_dump \
             bcmpkt_txpmd_len_get \
             bcmpkt_txpmd_field_get \
             bcmpkt_txpmd_field_set \
             bcmpkt_txpmd_field_name_get \
             bcmpkt_txpmd_field_id_get \
             bcmpkt_txpmd_fid_support_get \
             bcmpkt_txpmd_fid_view_get


KAPI_FUNCS_PREFIX = pktio
CFGFLAGS += $(foreach f,${KAPI_FUNCS},-D$f=${KAPI_FUNCS_PREFIX}_$f)

LKM_CFLAGS := $(CFGFLAGS) $(LKM_BUILD_INFO) -DPKTIO_KIMPL=1 -Dsal_memset=memset -Dsal_strcasecmp=strcasecmp
LKM_CPPFLAGS := $(CFGFLAGS) $(LKM_BUILD_INFO) -DPKTIO_KIMPL=1 -Dsal_memset=memset -Dsal_strcasecmp=strcasecmp
export LKM_CFLAGS
export LKM_CPPFLAGS

# SDK make helper for stand-alone PMD kernel module
include $(SDK)/make/Make.kpmd

distclean::
	rm -rf $(GENDIR)

endif # KPMD

include Kbuild

ifeq ($(KERNELRELEASE),)

MOD_NAME = linux-ngknetcb

include $(SDK)/make/Make.lkm

endif

.PHONY: distclean

distclean::
