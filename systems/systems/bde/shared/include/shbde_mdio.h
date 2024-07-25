/*
 * $Id: $
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
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
 *
 */

#ifndef __SHBDE_MDIO_H__
#define __SHBDE_MDIO_H__

#include <shbde.h>

typedef struct shbde_mdio_ctrl_s {

    /* Primary HAL*/
    shbde_hal_t *shbde;

    /* Context for iProc MDIO register access */
    void *regs;

    /* Base address for MDIO registers */
    unsigned int base_addr;

    /* iProc MDIO register access */
    unsigned int (*io32_read)(shbde_hal_t *shbde, void *iproc_regs, 
                              unsigned int addr);
    void (*io32_write)(shbde_hal_t *shbde, void *iproc_regs, 
                       unsigned int addr, unsigned int data);

} shbde_mdio_ctrl_t;


extern int
shbde_iproc_mdio_init(shbde_mdio_ctrl_t *smc);

extern int
shbde_iproc_mdio_read(shbde_mdio_ctrl_t *smc, unsigned int phy_addr,
                      unsigned int reg, unsigned int *val);

extern int
shbde_iproc_mdio_write(shbde_mdio_ctrl_t *smc, unsigned int phy_addr,
                       unsigned int reg, unsigned int val);

#endif /* __SHBDE_MDIO_H__ */
