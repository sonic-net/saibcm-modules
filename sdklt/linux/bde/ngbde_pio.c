/*! \file ngbde_pio.c
 *
 * API for managing and accessing memory-mapped I/O for switch
 * registers.
 *
 */
/*
 *
 * Copyright 2018-2026 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#include <ngbde.h>

bool
ngbde_pio_win_valid(void *devh, phys_addr_t addr, phys_addr_t size, char *debug)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;
    struct ngbde_memwin_s *iowin;
    unsigned long iowin_size;
    unsigned int wdx;

    for (wdx = 0; wdx < NGBDE_NUM_IOWIN_MAX; wdx++) {
        iowin = &sd->iowin[wdx];
        iowin_size = iowin->size;
        if (iowin_size < PAGE_SIZE) {
            iowin_size = PAGE_SIZE;
        }
        if (debug) {
            printk("%s: Check 0x%08lx/0x%08lx against "
                   "0x%08lx/0x%08lx(0x%08lx)\n",
                   debug,
                   (unsigned long)addr, (unsigned long)size,
                   (unsigned long)iowin->addr,
                   (unsigned long)iowin->size, iowin_size);
        }
        if (addr >= iowin->addr &&
            (addr + size) <= (iowin->addr + iowin_size)) {
            return true;
        }
    }
    return false;
}

void *
ngbde_pio_map(void *devh, phys_addr_t addr, phys_addr_t size)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        if (addr == sd->pio_win.addr && size == sd->pio_win.size) {
            /* Already mapped */
            return sd->pio_mem;
        }
        ngbde_pio_unmap(devh);
    }

    sd->pio_mem = ioremap(addr, size);

    if (sd->pio_mem) {
        /* Save mapped resources */
        sd->pio_win.addr = addr;
        sd->pio_win.size = size;
    } else {
        printk(KERN_WARNING "%s: Unable to map address 0x%08lu\n",
               MOD_NAME, (unsigned long)addr);
    }

    return sd->pio_mem;
}

void
ngbde_pio_unmap(void *devh)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        iounmap(sd->pio_mem);
        sd->pio_mem = NULL;
    }
}

void
ngbde_pio_cleanup(void)
{
    struct ngbde_dev_s *swdev, *sd;
    unsigned int num_swdev, idx;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        sd = ngbde_swdev_get(idx);
        if (sd) {
            ngbde_pio_unmap(sd);
        }
    }
}

void
ngbde_pio_write32(void *devh, uint32_t offs, uint32_t val)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        NGBDE_IOWRITE32(val, sd->pio_mem + offs);
    }
}

uint32_t
ngbde_pio_read32(void *devh, uint32_t offs)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        return NGBDE_IOREAD32(sd->pio_mem + offs);
    }
    return 0;
}
