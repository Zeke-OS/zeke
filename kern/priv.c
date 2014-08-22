/**
 *******************************************************************************
 * @file    priv.c
 * @author  Olli Vanhoja
 * @brief   User credentials.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2009 Robert N. M. Watson
 * Copyright (c) 2006 nCircle Network Security, Inc.
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

#define KERNEL_INTERNAL
#include <proc.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <sys/priv.h>

static int  suser_enabled = 1;
SYSCTL_INT(_security, OID_AUTO, suser_enabled, CTLFLAG_RW,
        &suser_enabled, 0, "processes with uid 0 have privilege");
//TUNABLE_INT("security.suser_enabled", &suser_enabled);

/**
 * Test active securelevel.
 * Test whether or not the active security level is greater than the
 * supplied level.
 * @param level is the needed security level.
 * @return  Returns -EPERM if condition evaluated to true; Otherwise zero.
 */
int securelevel_gt(proc_info_t * proc, int level)
{
    return (proc->securelevel > level ? -EPERM : 0);
}

/**
 * Test active securelevel.
 * Test whether or not the active security level is greater than or equal to
 * the supplied level.
 * @param level is the needed security level.
 * @return  Returns -EPERM if condition evaluated to true; Otherwise zero.
 */
int securelevel_ge(proc_info_t * proc, int level)
{
    return (proc->securelevel >= level ? -EPERM : 0);
}

/*
 * Check a credential for privilege.  Lots of good reasons to deny privilege;
 * only a few to grant it.
 */
int
priv_check_cred(proc_info_t * proc, int priv, int flags)
{
    int error;

    /*
     * Having determined if privilege is restricted by various policies,
     * now determine if privilege is granted.  At this point, any policy
     * may grant privilege.  For now, we allow short-circuit boolean
     * evaluation, so may not call all policies.  Perhaps we should.
     *
     * Superuser policy grants privilege based on the effective (or in
     * the case of specific privileges, real) uid being 0.  We allow the
     * superuser policy to be globally disabled, although this is
     * currenty of limited utility.
     */
    if (suser_enabled) {
        switch (priv) {
        case PRIV_MAXFILES:
        case PRIV_MAXPROC:
        case PRIV_PROC_LIMIT:
            if (proc->uid == 0) {
                error = 0;
                goto out;
            }
            break;
        default:
            if (proc->euid == 0) {
                error = 0;
                goto out;
            }
            break;
        }
    }

    /*
     * Writes to kernel/physical memory are a typical root-only operation,
     * but non-root users are expected to be able to read it (provided they
     * have permission to access /dev/[k]mem).
     */
    if (priv == PRIV_KMEM_READ) {
        error = 0;
        goto out;
    }

    /*
     * The default is deny, so if no policies have granted it, reject
     * with a privilege error here.
     */
    error = -EPERM;
out:
    return error;
}

int priv_check(proc_info_t * proc, int priv)
{
    return priv_check_cred(proc, priv, 0);
}
