# -*- Makefile -*-
# $Id: Makefile,v 1.3 Broadcom SDK $
#
# $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
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
# BCM Generic Netlink Interface
#
LOCALDIR = systems/linux/kernel/modules/bcm-genl

include ${SDK}/make/Make.config

ifndef DCBDIR
DCBDIR = $(SDK)/systems/linux/kernel/modules/dcb
# $(error 'The $$DCBDIR variable is not set')
endif
LKM_CPPFLAGS += -DDCBLIB_SUPPORT -I$(DCBDIR)/include
# Pass the same chip-support flags used by the DCB library standalone Makefile,
# so that BCM_*_SUPPORT defines (e.g. BCM_TRIDENT3_SUPPORT for BCM56873) are
# available when the DCB sources are compiled in the kernel module context.
LKM_CPPFLAGS += $(shell grep "^DFLAGS +=" $(DCBDIR)/make/Makefile | awk '{print $$3}')
DCB_VSRCS := $(shell find $(DCBDIR) -name test -prune -o -name \*.c -print)
DCB_LSRCS := $(notdir $(DCB_VSRCS))
DCB_OBJS := $(DCB_LSRCS:.c=.o)
ifeq (1,$(BUILD_GENL_PACKET))
LKM_CPPFLAGS += -I${SDK}/systems/linux/kernel/modules/include/kernel
endif

ifdef LTSW_CHIPS
# bcmdrd_config.h is required for enum_max.h when LTSW devices are included
LKM_CPPFLAGS += -I${SDKLT}/bcmdrd/include
endif

GENL_LHDRS := $(wildcard *.h)
GENL_LSRCS := $(wildcard *.c)
GENL_OBJS := $(GENL_LSRCS:.c=.o)

export LKM_CPPFLAGS

export MOD_NAME := linux-bcm-genl

export MOD_OBJS := \
	$(GENL_OBJS) \
	$(DCB_OBJS) \
	gmodule.o

KMODULE = $(LIBDIR)/$(MOD_NAME).ko

build: $(KMODULE)

#
# KBUILD_EXTRA_SYMBOLS exports the symbols created on the first module
# compilation to all other module compilations.
#
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../bcm-knet/Module.symvers
ifeq (1,$(BUILD_GENL_PACKET))
KBUILD_EXTRA_SYMBOLS += ${BLDDIR}/../genl-packet/Module.symvers
endif

#
# Suppress symlink error messages.
#
# Note that we do not use "ln -f" as this may cause failures if
# multiple builds are done in parallel on the same source tree.
#
R = 2>/dev/null

mklinks:
	mkdir -p $(BLDDIR)
	rm -f $(BLDDIR)/*.c
	rm -f $(BLDDIR)/*.h
	rm -f $(BLDDIR)/Makefile
	-ln -sr ${SDK}/make/Makefile.linux-kmodule $(BLDDIR)/Makefile $(R)
	-ln -sr bcm-genl.c $(BLDDIR) $(R)
	-for f in $(GENL_LHDRS); do ln -sr $$f $(BLDDIR) $(R); done
	-for f in $(GENL_LSRCS); do ln -sr $$f $(BLDDIR) $(R); done
	-for f in $(DCB_VSRCS); do ln -sr $$f $(BLDDIR) $(R); done
	-ln -sr ../../../../linux/kernel/modules/shared/gmodule.c $(BLDDIR) $(R)

$(KMODULE): mklinks
	$(MAKE) -C $(BLDDIR) KBUILD_EXTRA_SYMBOLS="$(KBUILD_EXTRA_SYMBOLS)"

clean::
	rm -f $(KMODULE)
	rm -rf $(BLDDIR)

distclean::

.PHONY: build mklinks $(KMODULE)
