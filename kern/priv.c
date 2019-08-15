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

const char * priv_cap_name[_PRIV_MENT] = {
    PRIV_FOREACH_CAP(PRIV_GENERATE_CAP_STRING_ARRAY)
};

/**
 * Default capabilities of a process.
 */
static int default_privs[] = {
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
    PRIV_PROC_FORK,
    PRIV_SIGNAL_ACTION,
    0
};

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

int priv_cred_eff_get(const struct cred * cred, int priv)
{
    return bitmap_status(cred->pcap_effmap, priv, _PRIV_MLEN);
}

int priv_cred_eff_set(struct cred * cred, int priv)
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

int priv_cred_eff_clear(struct cred * cred, int priv)
{
    return bitmap_clear(cred->pcap_effmap, priv, _PRIV_MLEN);
}

int priv_cred_bound_get(const struct cred * cred, int priv)
{
    return bitmap_status(cred->pcap_bndmap, priv, _PRIV_MLEN);
}

int priv_cred_bound_set(struct cred * cred, int priv)
{
    return bitmap_set(cred->pcap_bndmap, priv, _PRIV_MLEN);
}

int priv_cred_bound_clear(struct cred * cred, int priv)
{
    return bitmap_clear(cred->pcap_bndmap, priv, _PRIV_MLEN);
}

static void priv_cred_bound_reset(struct cred * cred)
{
    int * priv;
    int err;

    err = bitmap_block_update(cred->pcap_bndmap, 0, 0, _PRIV_MENT, _PRIV_MLEN);
    KASSERT(err == 0, "clear all bounding caps");

    for (priv = default_privs; *priv; priv++) {
        int v = *priv;

        priv_cred_bound_set(cred, v);
    }
}

void priv_cred_init(struct cred * cred)
{
    size_t i = 0;
    gid_t * gid = cred->sup_gid;
    int * priv;

    /*
     * Clear supplementary groups.
     */
    for (i = 0; i < NGROUPS_MAX; i++) {
        gid[i] = NOGROUP;
    }

    /* First make syre the capability maps are clear. */
    (void)bitmap_block_update(cred->pcap_effmap, 0, 0, _PRIV_MENT, _PRIV_MLEN);
    (void)bitmap_block_update(cred->pcap_bndmap, 0, 0, _PRIV_MENT, _PRIV_MLEN);

    for (priv = default_privs; *priv; priv++) {
        int v = *priv;

        priv_cred_bound_set(cred, v);
        priv_cred_eff_set(cred, v);
    }
}

void priv_cred_init_fork(struct cred * cred)
{
    /* Clear effective capabilities that are not set in the bounding set. */
    for (size_t i = 0; i < _PRIV_MENT; i++) {
        if (priv_cred_bound_get(cred, i) == 0) {
            priv_cred_eff_clear(cred, i);
        }
    }
}

void priv_cred_init_exec(struct cred * cred)
{
    if (priv_cred_eff_get(cred, PRIV_EXEC_B2E)) {
        /* Copy the bounding set to the effective set. */
        memcpy(cred->pcap_effmap, cred->pcap_bndmap, _PRIV_MSIZE);
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
    int err;

    err = priv_check(fromcred, priv);
    if (err == 0 || err != -EPERM) {
        return err;
    }

    switch (priv) {
    case PRIV_SIGNAL_OTHER:
        if (!((fromcred->euid != tocred->uid &&
             fromcred->euid != tocred->suid) &&
            (fromcred->uid  != tocred->uid &&
             fromcred->uid  != tocred->suid))) {
            return 0;
        }
        break;
    default:
        if (fromcred->euid == tocred->euid) {
            return 0;
        }
    }

    return -EPERM;
}

/**
 * @return -1 if failed;
 *          0 if status was zero or operation succeed;
 *          greater than zero status was one.
 */
static intptr_t sys_priv_pcap(__user void * user_args)
{
    struct _priv_pcap_args args;
    struct cred * proccred;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        err = -EFAULT;
        goto out;
    }

    proccred = &curproc->cred;

    switch (args.mode) {
    case PRIV_PCAP_MODE_GET_EFF: /* Get effective */
        err = priv_cred_eff_get(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_SET_EFF: /* Set effective */
        err = priv_check(proccred, PRIV_SETEFF);
        if (err) {
            err = -EPERM;
            break;
        }

        err = priv_cred_eff_set(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_CLR_EFF: /* Clear effective */
        err = priv_check(proccred, PRIV_CLRCAP);
        if (err) {
            err = -EPERM;
            break;
        }

        err = priv_cred_eff_clear(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_GET_BND: /* Get bounding */
        err = priv_cred_bound_get(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_SET_BND: /* Set bounding */
        err = priv_check(proccred, PRIV_SETBND);
        if (err) {
            err = -EPERM;
            break;
        }

        err = priv_cred_bound_set(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_CLR_BND: /* Clear bounding */
        err = priv_check(proccred, PRIV_CLRCAP);
        if (err) {
            err = -EPERM;
            break;
        }

        priv_cred_bound_clear(proccred, args.priv);
        break;
    case PRIV_PCAP_MODE_RST_BND:
        /* Reset bounding capabilities to the default */
        err = priv_check(proccred, PRIV_SETBND);
        if (err) {
            err = -EPERM;
            break;
        }

        priv_cred_bound_reset(proccred);
        break;
    default:
        err = -EINVAL;
    }

out:
    if (err) {
        set_errno(-err);
        err = -1;
    }

    return err;
}

/**
 * @return -1 if failed;
 *          0 if status was zero or operation succeed;
 *          greater than zero status was one.
 */
static intptr_t sys_priv_pcap_getall(__user void * user_args)
{
    struct _priv_pcap_getall_args args;
    struct cred * proccred = &curproc->cred;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (args.effective) {
        err = copyout(proccred->pcap_effmap,
                      (__user void *)args.effective, _PRIV_MSIZE);
    }
    if (err == 0 && args.bounding) {
        err = copyout(proccred->pcap_bndmap,
                      (__user void *)args.bounding, _PRIV_MSIZE);
    }

    if (err) {
        set_errno(-err);
        return -1;
    }

    return 0;
}

static const syscall_handler_t priv_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_PRIV_PCAP, sys_priv_pcap),
    ARRDECL_SYSCALL_HNDL(SYSCALL_PRIV_PCAP_GETALL, sys_priv_pcap_getall),
};
SYSCALL_HANDLERDEF(priv_syscall, priv_sysfnmap)
