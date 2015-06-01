/**
 *******************************************************************************
 * @file    sysinfo.h
 * @author  Olli Vanhoja
 * @brief   Header file for sysinfo.
 * @section LICENSE
 * Copyright (c) 2013, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <machine/endian.h>
#include <kerror.h>
#include <sys/sysctl.h>
#include <hal/mmu.h> /* to get MMU_PGSIZE_COARSE */
#include <hal/sysinfo.h>

sysinfo_t sysinfo = {
    .mem.size = configDYNMEM_SAFE_SIZE,
    .console = "/dev/ttyS0",
    .root = configROOTFS_PATH " " configROOTFS_NAME,
};

/* TODO HW_MODEL */

SYSCTL_INT(_hw, HW_BYTEORDER, byteorder, CTLFLAG_RD, 0, _BYTE_ORDER,
              "Byte order");

/* TODO HW_NCPU */

SYSCTL_INT(_hw, HW_PHYSMEM, physmem, CTLFLAG_RD, &sysinfo.mem.size, 0,
           "Total memory");

/* TODO HW_USERMEM */

SYSCTL_INT(_hw, HW_PAGESIZE, pagesize, CTLFLAG_RD, 0, MMU_PGSIZE_COARSE,
           "Page size");

SYSCTL_INT(_hw, HW_FLOATINGPT, floatingpt, CTLFLAG_RD, 0, configHAVE_HFP,
           "Hardware floating point");

/* TODO HW_MACHINE_ARCH */

/* TODO HW_REALMEM but maybe not here? */

SYSCTL_STRING(_kern, OID_AUTO, root, CTLFLAG_RD, &sysinfo.root, 0,
              "Root fs and type");

static const char cmdline_console[] = "console=";
static const char cmdline_root[] = "root=";
static const char cmdline_rootfstype[] = "rootfstype=";

void sysinfo_setmem(size_t start, size_t size)
{
    sysinfo.mem.start = start;
    sysinfo.mem.size = size;
}

void sysinfo_cmdline(const char * cmdline)
{
    const char * s;
    const char * root;
    const char * rootfstype;

    s = strstr(cmdline, cmdline_console);
    if (s) {
        strlcpy(sysinfo.console, s + sizeof(cmdline_console),
                sizeof(sysinfo.console));
    }

    root = strstr(cmdline, cmdline_root);
    rootfstype = strstr(cmdline, cmdline_rootfstype);
    if (root && rootfstype)
        ksprintf(sysinfo.root, sizeof(sysinfo.root), "%s %s", root, rootfstype);
}
