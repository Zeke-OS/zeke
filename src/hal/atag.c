/**
 *******************************************************************************
 * @file    atag.c
 * @author  Olli Vanhoja
 * @brief   ATAG scanner.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

#include <kstring.h>
#include <kerror.h>
#include "atag.h"

/* ATAGs */
#define ATAG_NONE       0x00000000 /*!< End of list. */
#define ATAG_CORE       0x54410001 /*!< Begining of the list. */
#define ATAG_MEM        0x54410002 /*!< Describes a physical area of memory. */
#define ATAG_VIDEOTEXT  0x54410003 /*!< Describes a VGA text display. */
#define ATAG_RAMDISK    0x54410004 /*!< Ramdisk description. */
#define ATAG_INITRD2    0x54420005 /*!< Location of compressed ramdisk. */
#define ATAG_SERIAL     0x54410006 /*!< 64 bit board serial number. */
#define ATAG_REVISION   0x54410007 /*!< 32 bit board revision number. */
#define ATAG_VIDEOLFB   0x54410008 /*!< vesafb-type framebuffers init vals. */
#define ATAG_CMDLINE    0x54410009 /*!< Command line to pass to kernel. */


void atag_scan(uint32_t * atag_addr)
{
    uint32_t tag_value;
    uint32_t * atags;
    char msg[120];

    if (*atag_addr != ATAG_CORE) {
        KERROR(KERROR_WARN, "No ATAGs!");
        return;
    }

    for (atags = atag_addr; atags < (uint32_t *)0x8000; atags += 1) {
        switch(atags[1]) {
            case ATAG_CORE:
                (void)strcpy(msg, "[ATAG_CORE] flags: ");
                itoah32(msg + strlenn(msg, sizeof(msg)), atags[2]);

                (void)strnncat(msg, sizeof(msg), ", page size: ", 40);
                itoah32(msg + strlenn(msg, sizeof(msg)), atags[3]);

                (void)strnncat(msg, sizeof(msg), ", rootdev: ", 40);
                itoah32(msg + strlenn(msg, sizeof(msg)), atags[4]);

                KERROR(KERROR_LOG, msg);

                atags += atags[0]-1;
                break;
            case ATAG_MEM:
                (void)strcpy(msg, "[ATAG_MEM] size: ");
                itoah32(msg + strlenn(msg, sizeof(msg)), atags[2]);

                (void)strnncat(msg, sizeof(msg), ", start: ", 40);
                itoah32(msg + strlenn(msg, sizeof(msg)), atags[3]);

                atags += atags[0]-1;

                KERROR(KERROR_LOG, msg);
#if configMMU != 0
                mmu_memstart = (size_t)atags[3];
                mmu_memsize = (size_t)atags[2];
#endif
                break;
            case ATAG_VIDEOTEXT:
                atags += atags[0]-1;
                break;
            case ATAG_RAMDISK:
                atags += atags[0]-1;
                break;
            case ATAG_INITRD2:
                atags += atags[0]-1;
                break;
            case ATAG_SERIAL:
                atags += atags[0]-1;
                break;
            case ATAG_REVISION:
                atags += atags[0]-1;
                break;
            case ATAG_VIDEOLFB:
                atags += atags[0]-1;
                break;
            case ATAG_CMDLINE:
                atags += 2;

                (void)strcpy(msg, "[ATAG_CMDLINE] : ");
                (void)strnncat(msg, sizeof(msg), (char *)atags, 80);

                atags += atags[0]-1;
                break;
            default:
                break;
        }
    }
}

/**
  * @}
  */
