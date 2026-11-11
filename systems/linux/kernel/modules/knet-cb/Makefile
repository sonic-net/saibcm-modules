# -*- Makefile -*-
# $Id: Makefile,v 1.3 Broadcom SDK $
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
# BCM Network Device Callback Example
#
LOCALDIR = systems/linux/kernel/modules/knet-cb

include ${SDK}/make/Make.config

ifeq (1,$(BUILD_DCBLIB))
ifndef DCBDIR
$(error 'The $$DCBDIR variable is not set')
endif
LKM_CPPFLAGS := -DDCBLIB_SUPPORT -I$(DCBDIR)/include
DCB_VSRCS := $(shell find $(DCBDIR) -name test -prune -o -name \*.c -print)
DCB_LSRCS := $(notdir $(DCB_VSRCS))
DCB_OBJS := $(DCB_LSRCS:.c=.o)
endif

ifeq (1,$(BUILD_PSAMPLE))
LKM_CPPFLAGS += -DPSAMPLE_SUPPORT
PSAMPLE_OBJS := psample-cb.o
endif

export LKM_CPPFLAGS

export MOD_NAME := linux-knet-cb

export MOD_OBJS := \
	knet-cb.o \
	$(DCB_OBJS) \
	$(PSAMPLE_OBJS) \
	gmodule.o

KMODULE = $(LIBDIR)/$(MOD_NAME).ko

build: $(KMODULE)

#
# KBUILD_EXTRA_SYMBOLS exports the symbols created on the first module
# compilation to all other module compilations.
#
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../bcm-knet/Module.symvers

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
	-ln -sr knet-cb.c $(BLDDIR) $(R)
ifeq (1,$(BUILD_DCBLIB))
	-for f in $(DCB_VSRCS); do ln -sr $$f $(BLDDIR) $(R); done
endif
ifeq (1,$(BUILD_PSAMPLE))
	-ln -sr psample-cb.c $(BLDDIR) $(R)
	-ln -sr psample-cb.h $(BLDDIR) $(R)
endif
	-ln -sr ../../../../linux/kernel/modules/shared/gmodule.c $(BLDDIR) $(R)

$(KMODULE): mklinks
	$(MAKE) -C $(BLDDIR) KBUILD_EXTRA_SYMBOLS="$(KBUILD_EXTRA_SYMBOLS)"

clean::
	rm -f $(KMODULE)
	rm -rf $(BLDDIR)

distclean::

.PHONY: build mklinks $(KMODULE)
