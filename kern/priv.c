/**
 *******************************************************************************
 * @file    priv.c
 * @author  Olli Vanhoja
 * @brief   User credentials.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <sys/priv.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <syscall.h>
#include <bitmap.h>
#include <kstring.h>
#include <proc.h>

#ifdef configSUSER
#define SUSER_EN_DEFAULT 1
#else
#define SUSER_EN_DEFAULT 0
#endif

static int suser_enabled = SUSER_EN_DEFAULT;
SYSCTL_INT(_security, OID_AUTO, suser_enabled, CTLFLAG_RW,
           &suser_enabled, 0, "processes with uid 0 have privilege");

static int securelevel = configBOOT_SECURELEVEL;
SYSCTL_INT(_security, OID_AUTO, securelevel, CTLTYPE_INT|CTLFLAG_RW,
           &securelevel, 0, "Current secure level");

int securelevel_ge(int level)
{
    return (securelevel >= level ? -EPERM : 0);
}

int securelevel_gt(int level)
{
    return (securelevel > level ? -EPERM : 0);
}

int priv_grp_is_member(const struct cred * cred, gid_t gid)
{
    size_t i;

    if (cred->egid == gid)
        return 1;

    for (i = 0; i < NGROUPS_MAX; i++) {
        if (cred->sup_gid[i] == gid) {
            return 1;
        }
    }

    return 0;
}

static int priv_cred_eff_get(const struct cred * cred, int priv)
{
    return bitmap_status(cred->pcap_effmap, priv, _PRIV_MLEN);
}

static int priv_cred_eff_set(struct cred * cred, int priv)
{
    int bound = bitmap_status(cred->pcap_bndmap, priv, _PRIV_MLEN);

    if (bound < 0) {
        /* An error was returned.*/
        return bound;
    } else if (bound == 0) {
        return -EPERM;
    }

    return bitmap_set(cred->pcap_effmap, priv, _PRIV_MLEN);
}

static int priv_cred_eff_clear(struct cred * cred, int priv)
{
    return bitmap_clear(cred->pcap_effmap, priv, _PRIV_MLEN);
}

static int priv_cred_bound_get(const struct cred * cred, int priv)
{
    return bitmap_status(cred->pcap_bndmap, priv, _PRIV_MLEN);
}

/**
 * This should be only called internally as no process should be allowed to
 * extend the bounding capabilities.
 */
static int priv_cred_bound_set(struct cred * cred, int priv)
{
    return bitmap_set(cred->pcap_bndmap, priv, _PRIV_MLEN);
}

static int priv_cred_bound_clear(struct cred * cred, int priv)
{
    return bitmap_clear(cred->pcap_bndmap, priv, _PRIV_MLEN);
}

void priv_cred_init(struct cred * cred)
{
    size_t i = 0;
    gid_t * gid = cred->sup_gid;

    /*
     * Clear supplementary groups.
     */
    for (i = 0; i < NGROUPS_MAX; i++) {
        gid[i] = NOGROUP;
    }

    int privs[] = {
        /*
         * Default grants.
         * Some permissions are just needed for normal operation
         * but sometimes we wan't to restrict these too.
         */
        PRIV_CLRCAP,
        PRIV_TTY_SETA,
        PRIV_VFS_READ,
        PRIV_VFS_WRITE,
        PRIV_VFS_EXEC,
        PRIV_VFS_LOOKUP,
        PRIV_VFS_CHROOT,
        PRIV_VFS_STAT,
        /*
         * Super user grants.
         * Later on we can remove most of these privileges as we implement a
         * file system based capabilities for binaries.
         */
        /* Capabilities management */
        PRIV_SETEFF,
        PRIV_SETBND,
        /* Credential management */
        PRIV_CRED_SETUID,
        PRIV_CRED_SETEUID,
        PRIV_CRED_SETSUID,
        PRIV_CRED_SETGID,
        PRIV_CRED_SETEGID,
        PRIV_CRED_SETSGID,
        PRIV_CRED_SETGROUPS,
        /* Process caps */
        PRIV_PROC_SETLOGIN,
        /* IPC */
        PRIV_SIGNAL_OTHER,
        /* sysctl */
        PRIV_SYSCTL_WRITE,
        /* vfs */
        PRIV_VFS_ADMIN,
        PRIV_VFS_CHROOT,
        PRIV_VFS_MOUNT,
        0
    };
    int * priv;

    for (priv = privs; *priv; priv++) {
        priv_cred_eff_set(cred, *priv);
    }
}

/*
 * Check a credential for privilege. Lots of good reasons to deny privilege;
 * only a few to grant it.
 */
int priv_check(const struct cred * cred, int priv)
{
    int error;

    /*
     * TODO REMOVE
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
            if (cred->uid == 0) {
                error = 0;
                goto out;
            }
            break;
        default:
            if (cred->euid == 0) {
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

    /* Check if we should grant privilege. */
    error = priv_cred_eff_get(cred, priv);
    if (error < 0) {
        goto out;
    } else if (error > 0) {
        /* Grant */
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

int priv_check_cred(const struct cred * fromcred, const struct cred * tocred,
                    int priv)
{
    switch (priv) {
    /*
     * RFE should we make this possible if PRIV_SIGNAL_OTHER is set in fromcred?
     */
    case PRIV_SIGNAL_OTHER:
            if ((fromcred->euid != tocred->uid &&
                 fromcred->euid != tocred->suid) &&
                (fromcred->uid  != tocred->uid &&
                 fromcred->uid  != tocred->suid)) {
                return -EPERM;
            }
    default:
            return priv_check(fromcred, priv);
    }

    return 0;
}

int priv_cred_inherit(const struct cred * fromcred, struct cred * tocred)
{
    memcpy(tocred, fromcred, sizeof(struct cred));

    /* Clear effective capabilities that are not set in the bounding set. */
    for (size_t i = 0; i < _PRIV_MENT; i++) {
        if (priv_cred_bound_get(tocred, i) == 0) {
            priv_cred_eff_clear(tocred, i);
        }
    }

    return 0;
}

/**
 * @return -1 if failed;
 *          0 if status was zero or operation succeed;
 *          greater than zero status was one.
 */
static intptr_t sys_priv_pcap(__user void * user_args)
{
    struct _priv_pcap_args args;
    struct proc_info * proc;
    struct cred * proccred;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    proc = proc_ref(args.pid);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }
    proccred = &proc->cred;

    switch (args.mode) {
    case PRIV_PCAP_MODE_GET_EFF: /* Get effective */
        err = priv_cred_eff_get(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_SET_EFF: /* Set effective */
        err = priv_check(&curproc->cred, PRIV_SETEFF);
        if (err) {
            set_errno(EPERM);
            return -1;
        }

        err = priv_cred_eff_set(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_CLR_EFF: /* Clear effective */
        err = priv_check(&curproc->cred, PRIV_CLRCAP);
        if (err) {
            set_errno(EPERM);
            return -1;
        }

        err = priv_cred_eff_clear(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_GET_BND: /* Get bounding */
        err = priv_cred_bound_get(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_SET_BND: /* Set bounding */
        err = priv_check(&curproc->cred, PRIV_SETBND);
        if (err) {
            set_errno(EPERM);
            return -1;
        }

        err = priv_cred_bound_set(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_CLR_BND: /* Clear bounding */
        err = priv_check(&curproc->cred, PRIV_CLRCAP);
        if (err) {
            set_errno(EPERM);
            return -1;
        }

        priv_cred_bound_clear(proccred, args.priv);
        break;
    default:
        err = -EINVAL;
    }

    if (err) {
        set_errno(-err);
        err = -1;
    }

    proc_unref(proc);
    return err;
}

static const syscall_handler_t priv_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_PRIV_PCAP, sys_priv_pcap),
};
SYSCALL_HANDLERDEF(priv_syscall, priv_sysfnmap)
