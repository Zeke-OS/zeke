/**
 *******************************************************************************
 * @file    atag.c
 * @author  Olli Vanhoja
 * @brief   ATAG scanner.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kstring.h>
#include <kerror.h>
#include <hal/sysinfo.h>
#include <hal/atag.h>

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


/**
 * ATAG scanner.
 * @note This function is called before intializers.
 */
void atag_scan(uint32_t fw, uint32_t mtype, uint32_t * atag_addr)
{
    uint32_t * atags;
    char msg[120];

    sysinfo.mtype = mtype;

    if (*atag_addr != ATAG_CORE) {
        KERROR(KERROR_WARN, "No ATAGs!");
        return;
    }

    for (atags = atag_addr; atags < (uint32_t *)0x8000; atags += 1) {
        switch(atags[1]) {
        case ATAG_CORE:
            ksprintf(msg, sizeof(msg),
                    "[ATAG_CORE] flags: %x, page size: %u, rootdev: %u\n",
                    atags[2], atags[3], atags[4]);
            KERROR(KERROR_INFO, msg);

            atags += atags[0]-1;
            break;
        case ATAG_MEM:
            ksprintf(msg, sizeof(msg),
                    "[ATAG_MEM] size: %x, start: %x\n",
                    atags[2], atags[3]);
            KERROR(KERROR_INFO, msg);

            atags += atags[0]-1;

            sysinfo.mem.size = (size_t)atags[2];
            sysinfo.mem.start = (size_t)atags[3];
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

            ksprintf(msg, sizeof(msg),
                    "[ATAG_CMDLINE] : %s\n", (char *)atags);
            KERROR(KERROR_INFO, msg);

            atags += atags[0]-1;
            break;
        default:
            break;
        }
    }
}
