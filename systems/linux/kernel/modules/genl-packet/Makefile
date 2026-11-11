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
# Generic Netlink Packet Interface
#
LOCALDIR = systems/linux/kernel/modules/genl-packet

include ${SDK}/make/Make.config

export LKM_CPPFLAGS := -I${SDK}/systems/linux/kernel/modules/include/kernel

export MOD_NAME := genl-packet

#
# For proper symbol support, the main source file name must differ
# from the module name, so we change the hyphens to underscores before
# we compile.
#
export MOD_OBJS := \
	genl_packet.o

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
	-ln -sr genl-packet.c $(BLDDIR)/genl_packet.c $(R)

$(KMODULE): mklinks
	$(MAKE) -C $(BLDDIR) KBUILD_EXTRA_SYMBOLS="$(KBUILD_EXTRA_SYMBOLS)"

clean::
	rm -f $(KMODULE)
	rm -rf $(BLDDIR)

distclean::

.PHONY: build mklinks $(KMODULE)
