/**
 *******************************************************************************
 * @file    sysctl_api.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel sysctl API.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Copyright (c) 1993
 * The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/** @addtogroup Library_Functions
  * @{
  */

#include <hal/hal_core.h>
#include "syscalldef.h"
#include "syscall.h"
#include <kstring.h> /* TODO Should include string.h if compiling for user space */
#include <sys/sysctl.h>

/** @addtogroup sysctl
 *  @{
 */

int sysctl(int * name, unsigned int namelen, void * oldp, size_t * oldlenp,
        void * newp, size_t newlen)
{
    struct _sysctl_args args = {
        .name = name,
        .namelen = namelen,
        .old = oldp,
        .oldlenp = oldlenp,
        .new = newp,
        .newlen = newlen
    };

    return (int)syscall(SYSCALL_SYSCTL_SYSCTL, &args);
}

int sysctlnametomib(char * name, int * oidp)
{
    int qoid[2] = {0, 3}; /* Magic: name2oid lookup */
    int i;
    size_t j;

    j = CTL_MAXNAME * sizeof(int);
    i = sysctl(qoid, 2, oidp, &j, name, strlenn(name, 80));
    if (i < 0)
        return (i);
    j /= sizeof(int);

    return (j);
}

int sysctloidfmt(int * oid, int len, char * fmt, unsigned int * kind)
{
    int qoid[CTL_MAXNAME + 2];
    char buf[80];
    int i;
    size_t j;

    qoid[0] = 0; /* Magic:            */
    qoid[1] = 4; /* oid format lookup */
    memcpy(qoid + 2, oid, len * sizeof(int));

    j = sizeof(buf);
    i = sysctl(qoid, len + 2, buf, &j, 0, 0);
    if (i)
        return 1;

    if (kind)
        *kind = *(unsigned int *)buf;

    if (fmt)
        strcpy(fmt, (char *)(buf + sizeof(unsigned int)));

    return 0;
}

int sysctlgetnext(int * oid, int len, int * oidn, size_t * lenn)
{
    int name[CTL_MAXNAME + 2];

    name[0] = 0;
    name[1] = 2;
    if (len > 0) {
        memcpy(name + 2, oid, len * sizeof(int));
        len += 2;
    } else {
        name[2] = 1; /* CTL_KERN */
        len = 3;
    }

    return sysctl(name, len, oidn, lenn, 0, 0);
}

/**
  * @}
  */

/**
  * @}
  */
