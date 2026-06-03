/*
 * $Id: error.h,v 1.23 Broadcom SDK $
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
 *
 * This file defines common error codes to be shared between API layers.
 *
 * Its contents are not used directly by applications; it is used only
 * by header files of parent APIs which need to define error codes.
 */

#ifndef _SHR_ERROR_H
#define _SHR_ERROR_H

typedef enum {
    _SHR_E_NONE                 = 0,
    _SHR_E_INTERNAL             = -1,
    _SHR_E_MEMORY               = -2,
    _SHR_E_UNIT                 = -3,
    _SHR_E_PARAM                = -4,
    _SHR_E_EMPTY                = -5,
    _SHR_E_FULL                 = -6,
    _SHR_E_NOT_FOUND            = -7,
    _SHR_E_EXISTS               = -8,
    _SHR_E_TIMEOUT              = -9,
    _SHR_E_BUSY                 = -10,
    _SHR_E_FAIL                 = -11,
    _SHR_E_DISABLED             = -12,
    _SHR_E_BADID                = -13,
    _SHR_E_RESOURCE             = -14,
    _SHR_E_CONFIG               = -15,
    _SHR_E_UNAVAIL              = -16,
    _SHR_E_INIT                 = -17,
    _SHR_E_PORT                 = -18,
    _SHR_E_IO                   = -19,
    _SHR_E_ACCESS               = -20,
    _SHR_E_NO_HANDLER           = -21,
    _SHR_E_PARTIAL              = -22,

    _SHR_E_LIMIT                = -23           /* Must come last */
} _shr_error_t;

#define _SHR_ERRMSG_INIT        { \
        "Ok",                           /* E_NONE */ \
        "Internal error",               /* E_INTERNAL */ \
        "Out of memory",                /* E_MEMORY */ \
        "Invalid unit",                 /* E_UNIT */ \
        "Invalid parameter",            /* E_PARAM */ \
        "Table empty",                  /* E_EMPTY */ \
        "Table full",                   /* E_FULL */ \
        "Entry not found",              /* E_NOT_FOUND */ \
        "Entry exists",                 /* E_EXISTS */ \
        "Operation timed out",          /* E_TIMEOUT */ \
        "Operation still running",      /* E_BUSY */ \
        "Operation failed",             /* E_FAIL */ \
        "Operation disabled",           /* E_DISABLED */ \
        "Invalid identifier",           /* E_BADID */ \
        "No resources for operation",   /* E_RESOURCE */ \
        "Invalid configuration",        /* E_CONFIG */ \
        "Feature unavailable",          /* E_UNAVAIL */ \
        "Feature not initialized",      /* E_INIT */ \
        "Invalid port",                 /* E_PORT */ \
        "I/O error",                    /* E_IO */ \
        "Access error",                 /* E_ACCESS */ \
        "No handler",                   /* E_NO_HANDLER */ \
        "Partial success",              /* E_PARTIAL */ \
        "Unknown error"                 /* E_LIMIT */ \
        }

extern char *_shr_errmsg[];

#define _SHR_ERRMSG(r)          \
        _shr_errmsg[(((int)r) <= 0 && ((int)r) > _SHR_E_LIMIT) ? -(r) : -_SHR_E_LIMIT]

#define _SHR_E_SUCCESS(rv)              ((rv) >= 0)
#define _SHR_E_FAILURE(rv)              ((rv) < 0)

#endif  /* !_SHR_ERROR_H */
