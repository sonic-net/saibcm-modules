# -*- Makefile -*-
# $Id: Makefile,v 1.18 Broadcom SDK $
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
# Makefile for Linux kernel BDE
#
LOCALDIR = systems/bde/linux/kernel

# Make sure we build for the kernel if this is a user-mode build
ifneq ($(platform), )
override-target=linux-$(platform)
endif

include $(SDK)/make/Make.config

ifeq (1,$(THREAD_SANITIZER_IS_ACTIVE))
  $(warning ./systems/bde/linux/kernel/Makefile. Do not apply THREAD SANITIZER to kernel objects)
  override CFLAGS := $(subst -fsanitize=thread, ,$(CFLAGS))
endif

export MOD_NAME := linux-kernel-bde

#
# For proper symbol support, the main source file name must differ
# from the module name, so we change the hyphens to underscores before
# we compile.
#
export MOD_OBJS := \
	linux_kernel_bde.o \
	linux_dma.o \
	linux_shbde.o \
	shbde_pci.o \
	shbde_iproc.o \
	shbde_mdio.o \
	mpool.o \
	gmodule.o

KMODULE = $(LIBDIR)/$(MOD_NAME).ko

build: $(KMODULE)

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
	-ln -sr linux-kernel-bde.c $(BLDDIR)/linux_kernel_bde.c $(R)
	-ln -sr linux_dma.c $(BLDDIR) $(R)
	-ln -sr linux_shbde.c $(BLDDIR) $(R)
	-ln -sr linux_shbde.h $(BLDDIR) $(R)
	-ln -sr ../../shared/shbde_pci.c $(BLDDIR) $(R)
	-ln -sr ../../shared/shbde_iproc.c $(BLDDIR) $(R)
	-ln -sr ../../shared/shbde_mdio.c $(BLDDIR) $(R)
	-ln -sr ../shared/mpool.c $(BLDDIR) $(R)
	-ln -sr ../../../linux/kernel/modules/shared/gmodule.c $(BLDDIR) $(R)

$(KMODULE): mklinks
	$(MAKE) -C $(BLDDIR)

clean::
	rm -f $(KMODULE)
	rm -rf $(BLDDIR)

distclean::

.PHONY: build mklinks $(KMODULE)
