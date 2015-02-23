/**
 *******************************************************************************
 * @file    proccred.c
 * @author  Olli Vanhoja
 * @brief   Process credentials.
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

#include <stddef.h>
#include <sys/types.h>
#include <syscall.h>
#include <errno.h>
#include <sys/priv.h>
#include <unistd.h>

static const struct _proc_credctl_args ds_init = { -1, -1, -1, -1, -1, -1 };

#define SYS_GETCRED(var, cred) do {         \
    struct _proc_credctl_args ds = ds_init; \
    if (syscall(SYSCALL_PROC_CRED, &ds)) {  \
        /* TODO Shouldn't fail? */          \
        while (1);                          \
    }                                       \
    (var) = ds.cred;                        \
} while (0)

#define SYS_SETCRED(err, value, cred) do {  \
    struct _proc_credctl_args ds = ds_init; \
    ds.cred = (value);                      \
    err = syscall(SYSCALL_PROC_CRED, &ds);  \
} while (0)


uid_t getuid(void)
{
    uid_t retval;

    SYS_GETCRED(retval, ruid);

    return retval;
}

uid_t geteuid(void)
{
    uid_t retval;

    SYS_GETCRED(retval, euid);

    return retval;
}

gid_t getgid(void)
{
    gid_t retval;

    SYS_GETCRED(retval, rgid);

    return retval;
}

gid_t getegid(void)
{
    gid_t retval;

    SYS_GETCRED(retval, egid);

    return retval;
}

int setuid(uid_t uid)
{
    struct _proc_credctl_args ds = {
        .ruid = uid,
        .euid = uid,
        .suid = uid,
        .rgid = -1,
        .egid = -1,
        .sgid = -1
    };
    struct _proc_credctl_args ds_save = ds;
    int errno_save, err;

    if (uid < 0) {
        errno = EINVAL;
        return -1;
    }

    errno_save = errno;
    err = syscall(SYSCALL_PROC_CRED, &ds);
    if (err && errno == EPERM && ds.euid == ds_save.euid) {
        errno = errno_save;
        return 0;
    }

    return -1;
}

int seteuid(uid_t uid)
{
    int err;

    if (uid < 0) {
        errno = EINVAL;
        return -1;
    }

    SYS_SETCRED(err, uid, euid);
    if (err) {
        errno = EPERM;
        return -1;
    }

    return 0;
}

int setgid(gid_t gid)
{
    struct _proc_credctl_args ds = {
        .ruid = -1,
        .euid = -1,
        .suid = -1,
        .rgid = gid,
        .egid = gid,
        .sgid = gid
    };
    struct _proc_credctl_args ds_save = ds;
    int errno_save, err;

    if (gid < 0) {
        errno = EINVAL;
        return -1;
    }

    errno_save = errno;
    err = syscall(SYSCALL_PROC_CRED, &ds);
    if (err && errno == EPERM && ds.egid == ds_save.egid) {
        errno = errno_save;
        return 0;
    }

    return -1;
}

int setegid(gid_t gid)
{
    int err;

    if (gid < 0) {
        errno = EINVAL;
        return -1;
    }

    SYS_SETCRED(err, gid, egid);
    if (err) {
        errno = EPERM;
        return -1;
    }

    return 0;
}

#if 0
int setreuid(uid_t, uid_t)
{
}

int setregid(gid_t, gid_t)
{
}
#endif

int priv_setpcap(pid_t pid, int grant, size_t priv, int value)
{
    struct _priv_pcap_args args = {
        .pid = pid,
        .priv = priv
    };

    if (!grant) {
        if (value)
            args.mode = PRIV_PCAP_MODE_SETR;
        else
            args.mode = PRIV_PCAP_MODE_CLRR;
    } else {
        if (value)
            args.mode = PRIV_PCAP_MODE_SETG;
        else
            args.mode = PRIV_PCAP_MODE_CLRG;
    }

    return syscall(SYSCALL_PRIV_PCAP, &args);
}

int priv_getcap(pid_t pid, int grant, size_t priv)
{
    struct _priv_pcap_args args = {
        .pid = pid,
        .priv = priv
    };

    if (!grant) {
        args.mode = PRIV_PCAP_MODE_GETR;
    } else {
        args.mode = PRIV_PCAP_MODE_GETG;
    }

    return syscall(SYSCALL_PRIV_PCAP, &args);
}
