/**
 *******************************************************************************
 * @file    proc_sysctl.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
 * @section LICENSE
 * Copyright (c) 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define PROC_INTERNAL

#include <errno.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <proc.h>

SYSCTL_INT(_kern, OID_AUTO, nprocs, CTLFLAG_RD,
           &nprocs, 0, "Current number of processes");

SYSCTL_INT(_kern, KERN_MAXPROC, maxproc, CTLFLAG_RD,
           NULL, configMAXPROC, "Maximum number of processes");

static int proc2pstat(struct pstat * ps, pid_t pid)
{
    struct proc_info * proc = proc_ref(pid);
    if (!proc) {
        return -EINVAL;
    }

    *ps = (struct pstat){
        .pid = pid,
        .pgrp = proc->pgrp->pg_id,
        .sid = proc->pgrp->pg_session->s_leader,
        .ctty = get_ctty(proc),
        .ruid = proc->cred.uid,
        .euid = proc->cred.euid,
        .suid = proc->cred.suid,
        .rgid = proc->cred.gid,
        .egid = proc->cred.egid,
        .sgid = proc->cred.sgid,
        .utime = proc->tms.tms_utime,
        .stime = proc->tms.tms_stime,
        .brk_start = proc->brk_start,
        .brk_stop = proc->brk_stop,
    };
    strlcpy(ps->name, proc->name, sizeof(ps->name));

    proc_unref(proc);
    return 0;
}

static int proc_sysctl_pid(struct sysctl_oid * oidp, int * mib, int len,
                           struct sysctl_req * req)
{
    if (len == 0) { /* Get a list of all PIDs */
        int retval;
        pid_t * pids;

        pids = proc_get_pids_buffer();

        PROC_LOCK();
        proc_get_pids(pids);
        PROC_UNLOCK();

        retval = sysctl_handle_opaque(oidp, pids, configMAXPROC + 1, req);
        proc_release_pids_buffer(pids);

        return retval;
    }

    /* Get single proc info */

    if (len < 2) {
        return -EINVAL;
    }
    pid_t pid = mib[0];
    int opt = mib[1];

    switch (opt) {
    case KERN_PROC_PSTAT: {
        struct pstat ps;

        if (proc2pstat(&ps, pid))
            return -EINVAL;
        return sysctl_handle_opaque(oidp, &ps, sizeof(struct pstat), req);
    }
    case KERN_PROC_VMMAP:
    case KERN_PROC_FILEDESC:
    case KERN_PROC_NFDS:
    case KERN_PROC_GROUPS:
    case KERN_PROC_ENV:
    case KERN_PROC_ARGS:
    case KERN_PROC_RLIMIT:
    case KERN_PROC_SIGTRAMP:
    case KERN_PROC_CWD:
    default:
        return -EINVAL;
    }
}

static int proc_sysctl(SYSCTL_HANDLER_ARGS)
{
    int * mib = arg1;
    int len = arg2;

    if (len < 1)
        return -EINVAL;

    switch (mib[0]) {
    case KERN_PROC_PID:
        return proc_sysctl_pid(oidp, mib + 1, len - 1, req);
    case KERN_PROC_PGRP:
    case KERN_PROC_SESSION:
    case KERN_PROC_TTY:
    case KERN_PROC_UID:
    case KERN_PROC_RUID:
    case KERN_PROC_RGID:
    case KERN_PROC_GID:
    default:
        return -EINVAL;
    }

    return 0;
}
SYSCTL_NODE(_kern, KERN_PROC, proc, CTLFLAG_RW, proc_sysctl,
            "High kernel, proc, limits &c");
