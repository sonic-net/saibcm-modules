#
#  Copyright 2017 Broadcom
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License, version 2, as
#  published by the Free Software Foundation (the "GPL").
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License version 2 (GPLv2) for more details.
#
#  You should have received a copy of the GNU General Public License
#  version 2 (GPLv2) along with this source code.
#
# -*- Makefile -*-
# $Id: Makefile,v 1.3 2012/07/17 07:39:51 mlarsen Exp $
# $Copyright: (c) 2005 Broadcom Corp.
# All Rights Reserved.$
#
# PTP Clock Intrerface
#
LOCALDIR = systems/linux/kernel/modules/bcm-ptp-clock

include ${SDK}/make/Make.config

export MOD_NAME := linux-bcm-ptp-clock

export MOD_OBJS := \
	bcm-ptp-clock.o \
	gmodule.o

KMODULE = $(LIBDIR)/$(MOD_NAME).ko

build: $(KMODULE)

#
# KBUILD_EXTRA_SYMBOLS exports the symbols created on the first module
# compilation to all other module compilations.
#
KBUILD_EXTRA_SYMBOLS := ${BLDDIR}/../../../../bde/linux/kernel/Module.symvers
KBUILD_EXTRA_SYMBOLS += ${BLDDIR}/../bcm-knet/Module.symvers

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
	-ln -sr bcm-ptp-clock.c $(BLDDIR) $(R)
	-ln -sr ../../../../linux/kernel/modules/shared/gmodule.c $(BLDDIR) $(R)

$(KMODULE): mklinks
	$(MAKE) -C $(BLDDIR) KBUILD_EXTRA_SYMBOLS="$(KBUILD_EXTRA_SYMBOLS)"

clean::
	rm -f $(KMODULE)
	rm -rf $(BLDDIR)

distclean::

.PHONY: build mklinks $(KMODULE)
