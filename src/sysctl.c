/**
 *******************************************************************************
 * @file    sysctl.c
 * @author  Olli Vanhoja
 *
 * @brief   sysctl kernel code.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1982, 1986, 1989, 1993
 *        The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Quite extensively rewritten by Poul-Henning Kamp of the FreeBSD
 * project, to make these variables more userfriendly.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *        @(#)kern_sysctl.c        8.4 (Berkeley) 4/14/94
 */

#define KERNEL_INTERNAL
#include <stdint.h>
#include <errno.h>
#include <kstring.h>
#include <kmalloc.h>
#include <sys/sysctl.h>

struct sysctl_oid_list sysctl__children; /* root list */

/*
 * Default "handler" functions.
 */

/**
 * Integer handler.
 * Handle an int, signed or unsigned.
 * Two cases:
 * + a variable:  point arg1 at it.
 * + a constant:  pass it in arg2.
 */
int sysctl_handle_int(SYSCTL_HANDLER_ARGS)
{
    int tmpout, error = 0;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = *(int *)arg1;
    else
        tmpout = arg2;
    error = SYSCTL_OUT(req, &tmpout, sizeof(int));

    if (error || !req->newptr)
        goto out;

    if (!arg1)
        error = EPERM;
    else
        error = SYSCTL_IN(req, arg1, sizeof(int));

out:
    return error;
}

/*
 * Long handler.
 * Handle a long, signed or unsigned.
 * Two cases:
 *     a variable:  point arg1 at it.
 *     a constant:  pass it in arg2.
 */
int sysctl_handle_long(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    long tmplong;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmplong = *(long *)arg1;
    else
        tmplong = arg2;

    error = SYSCTL_OUT(req, &tmplong, sizeof(long));

    if (error || !req->newptr)
        goto out;

    if (!arg1)
        error = EPERM;
    else
        error = SYSCTL_IN(req, arg1, sizeof(long));

out:
    return error;
}

/**
 * uint32_t handler.
 * Handle a 32 bit int, signed or unsigned.
 * Two cases:
 *     a variable:  point arg1 at it.
 *     a constant:  pass it in arg2.
 */
int sysctl_handle_32(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    uint32_t tmpout;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = *(uint32_t *)arg1;
    else
        tmpout = arg2;
    error = SYSCTL_OUT(req, &tmpout, sizeof(uint32_t));

    if (error || !req->newptr)
        goto out;

    if (!arg1)
        error = EPERM;
    else
        error = SYSCTL_IN(req, arg1, sizeof(uint32_t));

out:
    return error;
}

/**
 * uint64_t handler.
 * Handle a 64 bit int, signed or unsigned.
 * Two cases:
 *     a variable:  point arg1 at it.
 *     a constant:  pass it in arg2.
 */
int sysctl_handle_64(SYSCTL_HANDLER_ARGS)
{
    int error = 0;
    uint64_t tmpout;

    /*
     * Attempt to get a coherent snapshot by making a copy of the data.
     */
    if (arg1)
        tmpout = *(uint64_t *)arg1;
    else
        tmpout = arg2;
    error = SYSCTL_OUT(req, &tmpout, sizeof(uint64_t));

    if (error || !req->newptr)
        goto out;

    if (!arg1)
        error = EPERM;
    else
        error = SYSCTL_IN(req, arg1, sizeof(uint64_t));

out:
    return error;
}

/*
 * Handle our generic '\0' terminated 'C' string.
 * Two cases:
 *         a variable string:  point arg1 at it, arg2 is max length.
 *         a constant string:  point arg1 at it, arg2 is zero.
 */

int sysctl_handle_string(SYSCTL_HANDLER_ARGS)
{
    int error=0;
    char *tmparg;
    size_t outlen;

    /*
     * Attempt to get a coherent snapshot by copying to a
     * temporary kernel buffer.
     */
retry:
    outlen = strlenn((char *)arg1, 80) + 1; /* TODO hardcoding this here is
                                             *      not very clever. */
    tmparg = kmalloc(outlen);

    if (strlcpy(tmparg, (char *)arg1, outlen) >= outlen) {
        kfree(tmparg);
        goto retry;
    }

    error = SYSCTL_OUT(req, tmparg, outlen);
    kfree(tmparg);

    if (error || !req->newptr)
        goto out;

    if ((req->newlen - req->newidx) >= arg2) {
        error = EINVAL;
    } else {
        arg2 = (req->newlen - req->newidx);
        error = SYSCTL_IN(req, arg1, arg2);
        ((char *)arg1)[arg2] = '\0';
    }

out:
    return error;
}
