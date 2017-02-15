/**
 *******************************************************************************
 * @file    kern_mib.h
 * @author  Olli Vanhoja
 * @brief   Kernel Management information base (MIB).
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types_pthread.h>
#include <host.h>
#include <kactype.h>
#include <kstring.h>

SYSCTL_NODE(, CTL_KERN, kern, CTLFLAG_RW, 0,
        "High kernel, proc, limits &c");
SYSCTL_NODE(, CTL_VM, vm, CTLFLAG_RW, 0,
        "Virtual memory");
SYSCTL_NODE(, CTL_DEBUG, debug, CTLFLAG_RW, 0,
        "Debugging");
SYSCTL_NODE(, CTL_MACHDEP, machdep, CTLFLAG_RW, 0,
        "machine dependent");
SYSCTL_NODE(, OID_AUTO, security, CTLFLAG_RW, 0,
        "Security");

#ifndef KERNEL_VERSION
#define KERNEL_VERSION "0.0.0"
#endif
#ifndef KERNEL_RELENAME
#define KERNEL_RELENAME ""
#endif
static const char osrelease[] = KERNEL_RELENAME" "KERNEL_VERSION;
SYSCTL_STRING(_kern, KERN_OSRELEASE, osrelease, CTLFLAG_RD,
        (char *)osrelease, 0,
        "Operating system release");

/* TODO Arch */
static const char version[] = "ARCH" " " __DATE__;
SYSCTL_STRING(_kern, KERN_VERSION, version, CTLFLAG_RD,
        (char *)version, 0,
        "Kernel version");

static const char compiler_version[] =  __VERSION__;
SYSCTL_STRING(_kern, OID_AUTO, compiler_version, CTLFLAG_RD,
        (char *)compiler_version, 0,
        "Version of compiler used to compile kernel");

static const char ostype[] = "Zeke";
SYSCTL_STRING(_kern, KERN_OSTYPE, ostype, CTLFLAG_RD,
        (char *)ostype, 0,
        "Operating system type");

SYSCTL_INT(_kern, OID_AUTO, hz, CTLFLAG_RD, 0, configSCHED_HZ,
        "Number of kernel clock ticks per second");

char hostname[MAXHOSTNAMELEN + 1] = "wopr";
static int kern_mib_hostname(SYSCTL_HANDLER_ARGS)
{
    char tmp_hostname[MAXHOSTNAMELEN + 1];
    int error;

    memcpy(tmp_hostname, hostname, sizeof(hostname));
    error = sysctl_handle_string(oidp, tmp_hostname, sizeof(tmp_hostname), req);
    if (!error && req->newptr) { /* Validate new hostname before setting it */
        size_t label_nch = 0;

        if (ka_isdigit(tmp_hostname[0]) || !ka_isalpha(tmp_hostname[0]))
            return EINVAL;

        for (size_t i = 0; i < sizeof(tmp_hostname); i++) {
            char c = tmp_hostname[i];

            if (c == '\0') {
                if (tmp_hostname[i - 1] == '-')
                    return EINVAL;
                break;
            }

            if (c == '.') {
                if (label_nch == 0)
                    return EINVAL;
                label_nch = 0;
                continue;
            }

            if (++label_nch > 63)
                return EINVAL;

            if (!(ka_isalnum(c) || (c == '-')))
                return EINVAL;

            if (i >= 253)
                return EINVAL;
        }

        memcpy(hostname, tmp_hostname, sizeof(tmp_hostname));
    }

    return error;
}

SYSCTL_PROC(_kern, KERN_HOSTNAME, hostname,
        CTLTYPE_STRING | CTLFLAG_RW | CTLFLAG_SECURE3,
        NULL, 0, kern_mib_hostname, "A", "System hostname");
