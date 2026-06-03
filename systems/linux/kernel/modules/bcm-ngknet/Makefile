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
# Linux KNET module.
#

LOCALDIR = systems/linux/kernel/modules/bcm-ngknet
PKTIO_IMPL = 1
include ${SDK}/make/Make.config

PKTIODIR = $(SDK)/src/bcm/common/pktio
CNETDIR = $(PKTIODIR)/bcmcnet
KNETDIR = $(SDK)/systems/linux/kernel/modules/bcm-ngknet
GENDIR = $(KNETDIR)/generated
SRCIDIR = $(CNETDIR)/include/bcmcnet
ifneq ($(SDK_OUTDIR),)
GENDIR = $(SDK_OUTDIR)/$(SDKBUILD)/bcm-ngknet/generated
endif
DSTIDIR = $(GENDIR)/include/bcmcnet

# Change comma-separated list to space-separated list
comma = ,
empty =
space = $(empty) $(empty)
spc_sep = $(subst $(comma),$(space),$1)

# Convert chip name to uppercase
chip_uc = $(subst a,A,$(subst b,B,$(subst c,C,$(subst m,M,$1))))

# Convert chip name to lowercase
chip_lc = $(subst A,a,$(subst B,b,$(subst C,c,$(subst M,m,$1))))

#
# If SDK_CHIPS is defined, then exclude any chip directory which is
# not part of this list.
#
KNET_CHIPS := $(subst $(PKTIODIR)/chip/,,$(wildcard $(PKTIODIR)/chip/bcm*))
ifdef SDK_CHIPS
# Create space-separated lowercase version of chip list
SDK_CHIPS_SPC := $(call spc_sep,$(SDK_CHIPS))
SDK_CHIPS_LC := $(call chip_lc,$(SDK_CHIPS_SPC))
# Configure build flags according to chip list
KNET_CHIPS := $(filter $(SDK_CHIPS_LC),$(KNET_CHIPS))
KNET_CPPFLAGS := CHIP_DEFAULT=0 $(addsuffix =1,$(call chip_uc,$(KNET_CHIPS)))
KNET_CPPFLAGS := $(addprefix -DBCMDRD_CONFIG_INCLUDE_,$(KNET_CPPFLAGS))
export KNET_CPPFLAGS
endif
#
# Redefine KAPI interface API name to avoid symbol conflict.
#
KAPI_FUNCS = ngbde_kapi_dma_dev_get \
             ngbde_kapi_dma_bus_to_virt \
             ngbde_kapi_dma_virt_to_bus \
             ngbde_kapi_pio_write32 \
             ngbde_kapi_pio_read32 \
             ngbde_kapi_pio_membase \
             ngbde_kapi_iio_write32 \
             ngbde_kapi_iio_read32 \
             ngbde_kapi_intr_connect \
             ngbde_kapi_intr_disconnect \
             ngbde_kapi_intr_mask_write \
             ngbde_kapi_knet_connect \
             ngbde_kapi_knet_disconnect \
             ngknet_dev_init_cb_register \
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
             ngknet_ptp_rx_config_set_cb_register \
             ngknet_ptp_rx_config_set_cb_unregister \
             ngknet_ptp_tx_config_set_cb_register \
             ngknet_ptp_tx_config_set_cb_unregister \
             ngknet_ptp_rx_hwts_get_cb_register \
             ngknet_ptp_rx_hwts_get_cb_unregister \
             ngknet_ptp_tx_hwts_get_cb_register \
             ngknet_ptp_tx_hwts_get_cb_unregister \
             ngknet_ptp_tx_meta_set_cb_register \
             ngknet_ptp_tx_meta_set_cb_unregister \
             ngknet_ptp_phc_index_get_cb_register \
             ngknet_ptp_phc_index_get_cb_unregister \
             ngknet_ptp_dev_ctrl_cb_register \
             ngknet_ptp_dev_ctrl_cb_unregister \
             ngknet_ptp_rx_pre_process_cb_register \
             ngknet_ptp_rx_pre_process_cb_unregister

KAPI_FUNCS_PREFIX = pktio
CFGFLAGS += $(foreach f,${KAPI_FUNCS},-D$f=${KAPI_FUNCS_PREFIX}_$f)

LKM_CFLAGS := $(CFGFLAGS) $(LKM_BUILD_INFO) -DPKTIO_KIMPL=1
LKM_CPPFLAGS := $(CFGFLAGS) $(LKM_BUILD_INFO) -DPKTIO_KIMPL=1
export LKM_CFLAGS
export LKM_CPPFLAGS

.PHONY: mklinks rmlinks

knet: mklinks
	$(Q)echo Building kernel module $(BLDDIR)
	$(MAKE) -C $(GENDIR) all

#
# Suppress symlink error messages.
#
# Note that we do not use "ln -f" as this may cause failures if
# multiple builds are done in parallel on the same source tree.
#
R = 2>/dev/null

mklinks:
	mkdir -p $(DSTIDIR)
	-ln -s $(PKTIODIR)/pktio_dep.h $(GENDIR)/pktio_dep.h $(R)
	-ln -s $(KNETDIR)/ngknet_dep.h $(DSTIDIR)/bcmcnet_dep.h $(R)
	-ln -s $(KNETDIR)/ngknet_buff.h $(DSTIDIR)/bcmcnet_buff.h $(R)
	-ln -s $(SRCIDIR)/bcmcnet_types.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_internal.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_core.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_dev.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_rxtx.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_cmicx.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_cmicr_acc.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_cmicr.h $(DSTIDIR) $(R)
	-ln -s $(SRCIDIR)/bcmcnet_cmicr2.h $(DSTIDIR) $(R)
	-ln -s $(PKTIODIR)/chip/*/*attach.c $(GENDIR) $(R)
	-ln -s $(CNETDIR)/hmi/cmicx/*.c $(GENDIR) $(R)
	-ln -s $(CNETDIR)/hmi/cmicr/*.c $(GENDIR) $(R)
	-ln -s $(CNETDIR)/main/bcmcnet_core.c $(GENDIR) $(R)
	-ln -s $(CNETDIR)/main/bcmcnet_dev.c $(GENDIR) $(R)
	-ln -s $(CNETDIR)/main/bcmcnet_rxtx.c $(GENDIR) $(R)
	-ln -s $(KNETDIR)/*.[ch] $(GENDIR) $(R)
	-ln -s $(KNETDIR)/ngknet_adapter.c $(GENDIR) $(R)
	-ln -s $(KNETDIR)/Makefile $(GENDIR) $(R)
	-ln -s $(KNETDIR)/Kbuild $(GENDIR) $(R)

rmlinks:
	-rm -rf $(GENDIR)

CHIP_SRCS := $(addsuffix _pdma_attach.c,$(KNET_CHIPS))
CHIP_OBJS ?= $(patsubst %.c, %.o, $(CHIP_SRCS))
export CHIP_OBJS

include Kbuild

ifeq ($(KERNELRELEASE),)

MOD_NAME = linux-ngknet

include $(SDK)/make/Make.lkm
endif

.PHONY: distclean

distclean:: rmlinks
