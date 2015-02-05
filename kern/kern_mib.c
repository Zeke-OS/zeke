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

#include <autoconf.h>
#include <sys/types_pthread.h>
#include <sys/sysctl.h>

SYSCTL_NODE(, 0, sysctl, CTLFLAG_RW, 0,
        "Sysctl internal magic");
SYSCTL_NODE(, CTL_KERN, kern, CTLFLAG_RW|CTLFLAG_CAPRD, 0,
        "High kernel, proc, limits &c");
SYSCTL_NODE(, CTL_VM, vm, CTLFLAG_RW, 0,
        "Virtual memory");
SYSCTL_NODE(, CTL_DEBUG, debug, CTLFLAG_RW, 0,
        "Debugging");
SYSCTL_NODE(, CTL_HW, hw, CTLFLAG_RW, 0,
        "hardware");
SYSCTL_NODE(, CTL_MACHDEP, machdep, CTLFLAG_RW, 0,
        "machine dependent");
SYSCTL_NODE(, CTL_USER, user, CTLFLAG_RW, 0,
        "user-level");
SYSCTL_NODE(, OID_AUTO, security, CTLFLAG_RW, 0,
        "Security");

#ifndef KERNEL_VERSION
#define KERNEL_VERSION "0.0.0"
#endif
static const char osrelease[] = KERNEL_VERSION;
SYSCTL_STRING(_kern, KERN_OSRELEASE, osrelease, CTLFLAG_RD|CTLFLAG_MPSAFE,
        (char *)osrelease, 0,
        "Operating system release");

/* TODO Arch */
static const char version[] = "ARCH" " " __DATE__;
SYSCTL_STRING(_kern, KERN_VERSION, version, CTLFLAG_RD|CTLFLAG_MPSAFE,
        (char *)version, 0,
        "Kernel version");

static const char compiler_version[] =  __VERSION__;
SYSCTL_STRING(_kern, OID_AUTO, compiler_version, CTLFLAG_RD|CTLFLAG_MPSAFE,
        (char *)compiler_version, 0,
        "Version of compiler used to compile kernel");

static const char ostype[] = "Zeke";
SYSCTL_STRING(_kern, KERN_OSTYPE, ostype, CTLFLAG_RD|CTLFLAG_MPSAFE,
        (char *)ostype, 0,
        "Operating system type");

SYSCTL_INT(_kern, OID_AUTO, hz, CTLFLAG_RD, 0, configSCHED_HZ,
        "Number of kernel clock ticks per second");
