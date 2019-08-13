/**
 *******************************************************************************
 * @file    proc_fork.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/sysctl.h>
#include <buf.h>
#include <kerror.h>
#include <kinit.h>
#include <kstring.h>
#include <libkern.h>
#include <mempool.h>
#include <proc.h>

#ifdef configCOW_ENABLED
#define COW_ENABLED_DEFAULT 1
#else
#define COW_ENABLED_DEFAULT 0
#endif

struct mempool * proc_pool;
/** Enable copy on write for processses. */
static int cow_enabled = COW_ENABLED_DEFAULT;
static pid_t proc_lastpid;  /*!< last allocated pid. */

SYSCTL_BOOL(_kern, OID_AUTO, cow_enabled, CTLFLAG_RW,
            &cow_enabled, 0, "Enable copy on write for proc");

int _proc_init_fork(void)
{
    proc_pool = mempool_init(MEMPOOL_TYPE_NONBLOCKING,
                             sizeof(struct proc_info),
                             configMAXPROC);
    if (!proc_pool)
        return -ENOMEM;
    return 0;
}

static int clone_code_region(struct proc_info * new_proc,
                             struct proc_info * old_proc)
{
    struct buf * vm_reg_tmp;

    /* Copy code region pointer. */
    vm_reg_tmp = (*old_proc->mm.regions)[MM_CODE_REGION];
    if (!vm_reg_tmp) {
        KERROR(KERROR_ERR, "Old proc code region can't be null\n");
        return -EINVAL; /* Not allowed but this shouldn't happen. */
    }

    /*
     * In Zeke we always have at least one read-only code region by design so
     * we don't have to COW it or do anything fancy with it, instead we just
     * take a reference to the code region of the old process.
     */
    if (vm_reg_tmp->vm_ops->rref)
        vm_reg_tmp->vm_ops->rref(vm_reg_tmp);

    (*new_proc->mm.regions)[MM_CODE_REGION] = vm_reg_tmp;

    return 0;
}

static int clone_regions_from(struct proc_info * new_proc,
                              struct proc_info * old_proc,
                              int index)
{
    /*
     * Copy other region pointers from index to nr_regions.
     * As an iteresting sidenote, what we are doing here and earlier when L1
     * page table was cloned is that we are removing a link between the region
     * structs and the actual L1 page table of this process. However it doesn't
     * matter much since we are doing COW anyway so no information is ever
     * completely lost but we have to just keep in mind that COW regions are bit
     * incomplete and L1 is not completely reconstructable by using just
     * buf struct.
     *
     * Many BSD variants have fully reconstructable L1 tables but we don't have
     * it directly that way because shared regions can't properly point to more
     * than one page table struct.
     */
    for (int i = index; i < old_proc->mm.nr_regions; i++) {
        struct buf * vm_reg_tmp;
        int err;

        vm_reg_tmp = (*old_proc->mm.regions)[i];
        if (!vm_reg_tmp || (vm_reg_tmp->b_flags & B_NOTSHARED))
            continue;

        /* Take a ref */
        if (vm_reg_tmp->vm_ops->rref)
            vm_reg_tmp->vm_ops->rref(vm_reg_tmp);

        /* Don't clone regions in system page table */
        if (vm_reg_tmp->b_mmu.vaddr <= configKERNEL_END) {
            (*new_proc->mm.regions)[i] = vm_reg_tmp;
            continue;
        }

        /*
         * If the region is writable we want to either clone it or mark it as
         * copy-on-write.
         */
        if (vm_reg_tmp->b_uflags & VM_PROT_WRITE) {
            if (cow_enabled) { /* Set COW bit if the feature is enabled. */
                vm_reg_tmp->b_uflags |= VM_PROT_COW;

                /*
                 * Remap the region to old_proc to apply VM_PROT_COW.
                 */
                err = vm_mapproc_region(old_proc, vm_reg_tmp);
                if (err) {
                    KERROR(KERROR_ERR,
                           "Error while remapping a region for old_rpc (%d)\n",
                            err);
                }
            } else { /* copy immediately */
                if (vm_reg_tmp->vm_ops->rclone) {
                    struct buf * old_bp = vm_reg_tmp;
                    struct buf * new_bp;

                    /*
                     * Ref is no more needed, we can actually remove it early
                     * since its not going anywhere during the fork.
                     */
                    if (old_bp->vm_ops->rfree)
                        old_bp->vm_ops->rfree(old_bp);

                    new_bp = old_bp->vm_ops->rclone(old_bp);
                    if (!new_bp) {
                        KERROR(KERROR_ERR,
                               "Failed to clone a memory region (%p)\n",
                               old_bp);
                        continue;
                    }

                    vm_reg_tmp = new_bp;
                } else {
                    KERROR(KERROR_ERR, "Can't clone a memory region (%p)\n",
                           vm_reg_tmp);
                    continue;
                }
            }
        }
        (*new_proc->mm.regions)[i] = vm_reg_tmp;

        /*
         * Map the region to new_proc.
         */
        err = vm_mapproc_region(new_proc, vm_reg_tmp);
        if (err) {
            KERROR(KERROR_ERR,
                   "Error while mapping a region to new_proc (%d)\n",
                   err);
        }
    }

    return 0;
}

/**
 * Clone old process descriptor.
 */
static struct proc_info * clone_proc_info(struct proc_info * const old_proc)
{
    struct proc_info * new_proc;

    KERROR_DBG("clone proc info of pid %u\n", old_proc->pid);

    new_proc = mempool_get(proc_pool);
    if (!new_proc) {
        return NULL;
    }
    memcpy(new_proc, old_proc, sizeof(struct proc_info));

    return new_proc;
}

static int clone_stack(struct proc_info * new_proc, struct proc_info * old_proc)
{
    struct buf * const old_region = (*old_proc->mm.regions)[MM_STACK_REGION];
    struct buf * new_region;
    int err;

    if (unlikely(!old_region)) {
        KERROR_DBG("No stack created\n");
        return 0;
    }

    err = clone2vr(old_region, &new_region);
    if (err)
        return err;

    err = vm_replace_region(new_proc, new_region, MM_STACK_REGION,
                            VM_INSOP_MAP_REG);
    if (err) {
        new_region->vm_ops->rfree(new_region);
    }

    return err;
}

static void set_proc_inher(struct proc_info * old_proc,
                           struct proc_info * new_proc)
{
    KERROR_DBG("Updating inheriance attributes of new_proc\n");

    mtx_init(&new_proc->inh.lock, PROC_INH_LOCK_TYPE, PROC_INH_LOCK_OPT);
    new_proc->inh.parent = old_proc;
    PROC_INH_INIT(new_proc);

    mtx_lock(&old_proc->inh.lock);
    PROC_INH_INSERT_HEAD(old_proc, new_proc);
    mtx_unlock(&old_proc->inh.lock);
}

static pid_t proc_get_next_pid(void)
{
    const pid_t pid_reset = (configMAXPROC < 20) ? 2 :
                            (configMAXPROC < 200) ? configMAXPROC / 2 : 100;
    pid_t newpid = (proc_lastpid >= configMAXPROC) ? pid_reset :
                                                     proc_lastpid + 1;

    KERROR_DBG("%s()\n", __func__);

    PROC_LOCK();

    while (proc_exists_locked(newpid)) {
        newpid++;
        if (newpid > configMAXPROC) {
            newpid = pid_reset;
        }
    }
    proc_lastpid = newpid;

    PROC_UNLOCK();

    KERROR_DBG("%s done\n", __func__);

    return newpid;
}

pid_t proc_fork(void)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html
     */

    KERROR_DBG("%s(%u)\n", __func__, curproc->pid);

    struct proc_info * const old_proc = curproc;
    struct proc_info * new_proc;
    pid_t retval = 0;

    /* Check that the old process is in valid state. */
    if (!old_proc || old_proc->state == PROC_STATE_INITIAL)
        return -EINVAL;

    new_proc = clone_proc_info(old_proc);
    if (!new_proc)
        return -ENOMEM;

    /* Clear some things required to be zeroed at this point */
    new_proc->state = PROC_STATE_INITIAL;
    new_proc->exit_ksiginfo = NULL;
    new_proc->files = NULL;
    new_proc->pgrp = NULL; /* Must be NULL so we don't free the old ref. */
    memset(&new_proc->tms, 0, sizeof(new_proc->tms));
    /* ..and then start to fix things. */

    /*
     * Process group.
     */
    PROC_LOCK();
    proc_pgrp_insert(old_proc->pgrp, new_proc);
    PROC_UNLOCK();

    /*
     *  Initialize the mm struct.
     */
    retval = vm_mm_init(&new_proc->mm, old_proc->mm.nr_regions);
    if (retval)
        goto out;

    /*
     * Clone the master page table.
     * This is probably something we would like to get rid of but we are
     * stuck with because it's the easiest way to keep some static kernel
     * mappings valid between processes.
     */
    if (mmu_ptcpy(&new_proc->mm.mpt, &old_proc->mm.mpt)) {
        retval = -EAGAIN;
        goto out;
    }

    /*
     * Clone L2 page tables.
     */
    if (vm_ptlist_clone(&new_proc->mm.ptlist_head, &new_proc->mm.mpt,
                        &old_proc->mm.ptlist_head) < 0) {
        retval = -ENOMEM;
        goto out;
    }

    retval = clone_code_region(new_proc, old_proc);
    if (retval)
        goto out;

    /*
     * Clone stack region.
     */
    retval = clone_stack(new_proc, old_proc);
    if (retval) {
        KERROR_DBG("Cloning stack region failed.\n");
        goto out;
    }

    /*
     *  Clone other regions.
     */
    retval = clone_regions_from(new_proc, old_proc, MM_HEAP_REGION);
    if (retval)
        goto out;

    /*
     * Set break values.
     */
    new_proc->brk_start = (void *)(
            (*new_proc->mm.regions)[MM_HEAP_REGION]->b_mmu.vaddr +
            (*new_proc->mm.regions)[MM_HEAP_REGION]->b_bcount);
    new_proc->brk_stop = (void *)(
            (*new_proc->mm.regions)[MM_HEAP_REGION]->b_mmu.vaddr +
            (*new_proc->mm.regions)[MM_HEAP_REGION]->b_bufsize);

    /* fork() signals */
    ksignal_signals_fork_reinit(&new_proc->sigs);

    /*
     * Copy file descriptors.
     */
    KERROR_DBG("Copy file descriptors\n");
    int nofile_max = old_proc->rlim[RLIMIT_NOFILE].rlim_max;
    if (nofile_max < 0) {
#if configRLIMIT_NOFILE < 0
#error configRLIMIT_NOFILE can't be negative.
#endif
        nofile_max = configRLIMIT_NOFILE;
    }
    new_proc->files = fs_alloc_files(nofile_max, nofile_max);
    if (!new_proc->files) {
        KERROR_DBG(
               "\tENOMEM when tried to allocate memory for file descriptors\n");
        retval = -ENOMEM;
        goto out;
    }
    /* Copy and ref old file descriptors */
    for (int i = 0; i < old_proc->files->count; i++) {
        new_proc->files->fd[i] = old_proc->files->fd[i];
        fs_fildes_ref(new_proc->files, i, 1); /* null pointer safe */
    }
    KERROR_DBG("All file descriptors copied\n");

    /*
     * Select PID.
     */
    new_proc->pid = proc_get_next_pid();

    if (new_proc->cwd) {
        KERROR_DBG("Increment refcount for the cwd\n");
        vref(new_proc->cwd); /* Increment refcount for the cwd */
    }

    /* Update inheritance attributes */
    set_proc_inher(old_proc, new_proc);

    priv_cred_init_fork(&new_proc->cred);

    /* Insert the new process into the process array */
    procarr_insert(new_proc);

    /*
     * A process shall be created with a single thread. If a multi-threaded
     * process calls fork(), the new process shall contain a replica of the
     * calling thread.
     * We left main_thread null if calling process has no main thread.
     */
    KERROR_DBG("Handle main_thread\n");
    if (old_proc->main_thread) {
        KERROR_DBG("Call thread_fork() to get a new main thread for the fork.\n");
        if (!(new_proc->main_thread = thread_fork(new_proc->pid))) {
            KERROR_DBG("\tthread_fork() failed\n");
            retval = -EAGAIN;
            goto out;
        }

        KERROR_DBG("\tthread_fork() fork OK\n");

        /*
         * We set new proc's mpt as the current mpt because the new main thread
         * is going to return directly to the user space.
         */
        new_proc->main_thread->curr_mpt = &new_proc->mm.mpt;
        new_proc->state = PROC_STATE_READY;

        KERROR_DBG("Set the new main_thread (%d) ready\n",
                   new_proc->main_thread->id);
        thread_ready(new_proc->main_thread->id);
    } else {
        KERROR_DBG("No thread to fork.\n");
        new_proc->main_thread = NULL;
        new_proc->state = PROC_STATE_READY;
    }

    KERROR_DBG("Fork %d -> %d created.\n", old_proc->pid, new_proc->pid);
    retval = new_proc->pid;
out:
    if (unlikely(retval < 0)) {
        proc_free(new_proc);
    }
    return retval;
}
