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
#include <buf.h>
#include <kmalloc.h>
#include <proc.h>
#include <vm/vm.h>

SYSCTL_INT(_kern, OID_AUTO, nprocs, CTLFLAG_RD,
           &nprocs, 0, "Current number of processes");

SYSCTL_INT(_kern, KERN_MAXPROC, maxproc, CTLFLAG_RD,
           NULL, configMAXPROC, "Maximum number of processes");

static int proc2pstat(struct kinfo_proc * ps, struct proc_info * proc)
{
    *ps = (struct kinfo_proc){
        .pid = proc->pid,
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

    return 0;
}

static int proc_sysctl_pids(struct sysctl_oid * oidp, struct sysctl_req * req)
{
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

static int sysctl_proc_vmmap(struct sysctl_oid * oidp,
                             struct proc_info * proc,
                             struct sysctl_req * req)
{
    struct kinfo_vmentry * vmmap;
    struct vm_mm_struct * mm;
    size_t vmmap_size = 0;
    int retval;

    mm = &proc->mm;
    mtx_lock(&mm->regions_lock);

    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];

        if (region)
            vmmap_size += sizeof(struct kinfo_vmentry);
    }

    vmmap = kzalloc(vmmap_size);
    if (!vmmap) {
        retval = -ENOMEM;
        goto out;
    }

    struct kinfo_vmentry * entry = vmmap;
    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];

        if (!region)
            continue;

        *entry = (struct kinfo_vmentry){
            .paddr = region->b_data,
            .reg_start = region->b_mmu.vaddr,
            .reg_end = region->b_mmu.vaddr + region->b_bufsize - 1,
            .flags = region->b_flags,
        };
        vm_get_uapstring(entry->uap, region);
        entry++;
    }

    retval = sysctl_handle_opaque(oidp, vmmap, vmmap_size, req);

    retval = 0;
out:
    mtx_unlock(&mm->regions_lock);
    return retval;
}

static int proc_sysctl_pid(struct sysctl_oid * oidp, int * mib, int len,
                           struct sysctl_req * req)
{
    int retval = 0;

    if (len < 2) {
        return -EINVAL;
    }

    pid_t pid = mib[0] == -1 ? curproc->pid : mib[0];
    int opt = mib[1];

    struct proc_info * proc = proc_ref(pid);
    if (!proc) {
        return -ESRCH;
    }

    if (!(curproc->cred.euid == 0 || curproc->cred.euid == proc->cred.euid)) {
        retval = -ESRCH;
        goto out;
    }

    switch (opt) {
    case KERN_PROC_PSTAT: {
        struct kinfo_proc ps;

        if (proc2pstat(&ps, proc)) {
            retval = -ESRCH;
            goto out;
        }
        retval = sysctl_handle_opaque(oidp, &ps, sizeof(struct kinfo_proc),
                                      req);
    }
    break;
    case KERN_PROC_VMMAP:
        retval = sysctl_proc_vmmap(oidp, proc, req);
        break;
    case KERN_PROC_FILEDESC:
        /* TODO Implementation */
    case KERN_PROC_NFDS:
        /* TODO Implementation */
        retval = -EINVAL;
        break;
    case KERN_PROC_GROUPS:
        retval = sysctl_handle_opaque(oidp, &proc->cred.sup_gid,
                                      NGROUPS_MAX * sizeof(gid_t),
                                      req);
        break;
    case KERN_PROC_ENV:
        /* TODO Implementation */
    case KERN_PROC_ARGS:
        /* TODO Implementation */
    case KERN_PROC_RLIMIT:
        /* TODO Implementation */
    case KERN_PROC_SIGTRAMP:
        /* TODO Implementation */
    case KERN_PROC_CWD:
        /* TODO Implementation */
    default:
        retval = -EINVAL;
        break;
    }

out:
    proc_unref(proc);
    return retval;
}

static int proc_sysctl_sesssions(struct sysctl_oid * oidp,
                                 struct sysctl_req * req)
{
    int retval;

    PROC_LOCK();
    struct session * sp;

    TAILQ_FOREACH(sp, &proc_session_list_head, s_session_list_entry_) {
        struct kinfo_session s = {
            .s_leader = sp->s_leader,
            .s_ctty_fd = sp->s_ctty_fd,
        };
        strlcpy(s.s_login, sp->s_login, sizeof(s.s_login));

        retval = req->oldfunc(req, &s, sizeof(struct kinfo_session));
        if (retval < 0)
            goto out;
    }

    retval = 0;
out:
    PROC_UNLOCK();
    return retval;
}

static int proc_sysctl(SYSCTL_HANDLER_ARGS)
{
    int * mib = arg1;
    int len = arg2;

    if (len < 1)
        return -EINVAL;

    switch (mib[0]) {
    case KERN_PROC_PID:
        if (len == 1) { /* Get the list of all PIDs */
            return proc_sysctl_pids(oidp, req);
        } else { /* Get a single proc info */
            return proc_sysctl_pid(oidp, mib + 1, len - 1, req);
        }
    case KERN_PROC_PGRP:
        if (len == 1) { /* Get the list of process groups */
        } else { /* Get the list of PIDs in a process group */
        }
        /* TODO Implementation */
    case KERN_PROC_SESSION:
        if (len == 1) { /* Get the list of all sessions */
            return proc_sysctl_sesssions(oidp, req);
        } else { /* Get the list of pgrp identifiers in a session */
        }
        /* TODO Implementation */
    case KERN_PROC_TTY:
        /* TODO Implementation */
    case KERN_PROC_UID:
        /* TODO Implementation */
    case KERN_PROC_RUID:
        /* TODO Implementation */
    case KERN_PROC_RGID:
        /* TODO Implementation */
    case KERN_PROC_GID:
        /* TODO Implementation */
    default:
        return -EINVAL;
    }

    return 0;
}
SYSCTL_NODE(_kern, KERN_PROC, proc, CTLFLAG_RW, proc_sysctl,
            "High kernel, proc, limits &c");
