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
#include <autoconf.h>
#include <proc.h>
#include <errno.h>
#include <syscall.h>
#include <sys/sysctl.h>
#include <bitmap.h>
#include <sys/priv.h>

#ifdef configSUSER
#define SUSER_EN_DEFAULT 1
#else
#define SUSER_EN_DEFAULT 0
#endif

static int suser_enabled = SUSER_EN_DEFAULT;
SYSCTL_INT(_security, OID_AUTO, suser_enabled, CTLFLAG_RW,
           &suser_enabled, 0, "processes with uid 0 have privilege");
//TUNABLE_INT("security.suser_enabled", &suser_enabled);

/*
 * Check a credential for privilege. Lots of good reasons to deny privilege;
 * only a few to grant it.
 */
int
priv_check(struct proc_info * proc, int priv)
{
    int error;

#ifdef configPROCCAP
    /* Check if capability is disabled. */
    error = bitmap_status(proc->pcap_restrmap, priv, _PRIV_MLEN);
    if (error) {
        if (error != -EINVAL)
            error = -EPERM;
        goto out;
    }
#endif

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

#ifdef configPROCCAP
    /* Check if we should grant privilege. */
    error = bitmap_status(proc->pcap_grantmap, priv, _PRIV_MLEN);
    if (error < 0) {
        goto out;
    } else if (error > 0) {
        /* Grant */
        error = 0;
        goto out;
    }
#endif

    /*
     * The default is deny, so if no policies have granted it, reject
     * with a privilege error here.
     */
    error = -EPERM;
out:
    return error;
}

#ifdef configPROCCAP
/**
 * @return -1 if failed;
 *          0 if status was zero or operation succeed;
 *          greater than zero status was one.
 */
static int sys_priv_pcap(void * user_args)
{
    struct _ds_priv_pcap args;
    proc_info_t * proc;
    int err;

    err = priv_check(curproc, PRIV_ALTPCAP);
    if (err) {
        set_errno(EPERM);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    proc = proc_get_struct(args.pid);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }

    switch (args.mode) {
    case PRIV_PCAP_MODE_GETR: /* Get restr */
        err = bitmap_status(proc->pcap_restrmap, args.priv, _PRIV_MLEN);
        break;
    case PRIV_PCAP_MODE_SETR: /* Set restr */
        err = bitmap_set(proc->pcap_restrmap, args.priv, _PRIV_MLEN);
        break;
    case PRIV_PCAP_MODE_CLRR: /* Clear restr */
        err = bitmap_clear(proc->pcap_restrmap, args.priv, _PRIV_MLEN);
        break;
    case PRIV_PCAP_MODE_GETG: /* Get grant */
        err = bitmap_status(proc->pcap_grantmap, args.priv, _PRIV_MLEN);
        break;
    case PRIV_PCAP_MODE_SETG: /* Set grant */
        err = bitmap_set(proc->pcap_grantmap, args.priv, _PRIV_MLEN);
        break;
    case PRIV_PCAP_MODE_CLRG: /* Clear grant */
        err = bitmap_clear(proc->pcap_grantmap, args.priv, _PRIV_MLEN);
        break;
    default:
        set_errno(EINVAL);
        return -1;
    }

    return err;
}
#endif

static const syscall_handler_t priv_sysfnmap[] = {
#ifdef configPROCCAP
    ARRDECL_SYSCALL_HNDL(SYSCALL_PRIV_PCAP, sys_priv_pcap),
#else
    ARRDECL_SYSCALL_HNDL(SYSCALL_PRIV_PCAP, NULL),
#endif
};
SYSCALL_HANDLERDEF(priv_syscall, priv_sysfnmap)
