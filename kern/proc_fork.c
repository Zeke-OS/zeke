/**
 *******************************************************************************
 * @file    proc_fork.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fs/procfs.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>

#ifdef configCOW_ENABLED
#define COW_ENABLED_DEFAULT 1
#else
#define COW_ENABLED_DEFAULT 0
#endif

/** Enable copy on write for processses. */
static int cow_enabled = COW_ENABLED_DEFAULT;
static pid_t proc_lastpid;  /*!< last allocated pid. */

SYSCTL_INT(_kern, OID_AUTO, cow_enabled, CTLFLAG_RW,
           &cow_enabled, 0, "Enable copy on write for proc");

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
    struct buf * vm_reg_tmp;
    int err;

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

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "clone_proc_info of pid %u\n", old_proc->pid);
#endif

    new_proc = kmalloc(sizeof(struct proc_info));
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
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "fork(): No stack created\n");
#endif
        return 0;
    }

    err = clone2vr(old_region, &new_region);
    if (err)
        return err;

    err = vm_replace_region(new_proc, new_region, MM_STACK_REGION,
                            VM_INSOP_MAP_REG);
    return err;
}

static void set_proc_inher(struct proc_info * old_proc,
                           struct proc_info * new_proc)
{
#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "Updating inheriance attributes of new_proc\n");
#endif

    mtx_init(&new_proc->inh.lock, PROC_INH_LOCK_TYPE, 0);
    new_proc->inh.parent = old_proc;
    SLIST_INIT(&new_proc->inh.child_list_head);

    mtx_lock(&old_proc->inh.lock);
    SLIST_INSERT_HEAD(&old_proc->inh.child_list_head, new_proc,
                      inh.child_list_entry);
    mtx_unlock(&old_proc->inh.lock);
}

pid_t proc_get_random_pid(void)
{

    pid_t last_maxproc;
    pid_t newpid;
    int count = 0;

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "proc_get_random_pid()");
#endif

    PROC_LOCK();
    last_maxproc = act_maxproc;
    newpid = last_maxproc;

    /*
     * The new PID will be "randomly" selected between proc_lastpid and
     * maxproc
     */
    do {
        long d = last_maxproc - proc_lastpid - 1;

        if (d <= 1 || count == 20) {
            proc_lastpid = 2;
            count = 0;
            continue;
        }
        if (newpid + 1 > last_maxproc)
            newpid = proc_lastpid + kunirand(d);
        newpid++;
        count++;
    } while (proc_exists(newpid, PROC_LOCKED));

    proc_lastpid = newpid;

    PROC_UNLOCK();

#ifdef configPROC_DEBUG
    kputs(" done\n");
#endif

    return newpid;
}

pid_t proc_fork(void)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html
     */

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "fork(%u)\n", curproc->pid);
#endif

    struct proc_info * const old_proc = curproc;
    struct proc_info * new_proc;
    pid_t retval = 0;

    /* Check that the old PID was valid. */
    if (!old_proc || (old_proc->state == PROC_STATE_INITIAL)) {
        return -EINVAL;
    }

    new_proc = clone_proc_info(old_proc);
    if (!new_proc) {
        return -ENOMEM;
    }

    new_proc->pgrp = NULL; /* Must be NULL so we don't free the old ref. */
    PROC_LOCK();
    proc_pgrp_insert(old_proc->pgrp, new_proc);
    PROC_UNLOCK();

    retval = procarr_realloc();
    if (retval)
        goto out;

    /* Clear some things required to be zeroed at this point */
    new_proc->state = PROC_STATE_INITIAL;
    new_proc->files = NULL;
    /* ..and then start to fix things. */

    /* Initialize the mm struct */
    retval = vm_mm_init(&new_proc->mm, old_proc->mm.nr_regions);
    if (retval)
        goto out;

    /* Clone master page table.
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

    /* Clone stack region */
    retval = clone_stack(new_proc, old_proc);
    if (retval) {
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "Cloning stack region failed.\n");
#endif
        goto out;
    }

    /* Clone other regions */
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

    /* Copy file descriptors */
#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "Copy file descriptors\n");
#endif
    int nofile_max = old_proc->rlim[RLIMIT_NOFILE].rlim_max;
    if (nofile_max < 0) {
#if configRLIMIT_NOFILE < 0
#error configRLIMIT_NOFILE can't be negative.
#endif
        nofile_max = configRLIMIT_NOFILE;
    }
    new_proc->files = kmalloc(SIZEOF_FILES(nofile_max));
    if (!new_proc->files) {
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG,
               "\tENOMEM when tried to allocate memory for file descriptors\n");
#endif
        retval = -ENOMEM;
        goto out;
    }
    new_proc->files->count = nofile_max;
    /* Copy and ref old file descriptors */
    for (int i = 0; i < old_proc->files->count; i++) {
        new_proc->files->fd[i] = old_proc->files->fd[i];
        fs_fildes_ref(new_proc->files, i, 1); /* null pointer safe */
    }
#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "All file descriptors copied\n");
#endif

    /* Select PID */
    if (likely(nprocs != 1)) { /* Tecnically it would be good idea to have lock
                                * on nprocs before reading it but I think this
                                * should work fine... */
        new_proc->pid = proc_get_random_pid();
    } else { /* Proc is init */
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "Assuming this process to be init\n");
#endif
        new_proc->pid = 1;
    }

    if (new_proc->cwd) {
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "Increment refcount for the cwd\n");
#endif
        vref(new_proc->cwd); /* Increment refcount for the cwd */
    }

    /* Update inheritance attributes */
    set_proc_inher(old_proc, new_proc);

    /* Insert the new process into the process array */
    procarr_insert(new_proc);

    /*
     * A process shall be created with a single thread. If a multi-threaded
     * process calls fork(), the new process shall contain a replica of the
     * calling thread.
     * We left main_thread null if calling process has no main thread.
     */
#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "Handle main_thread\n");
#endif
    if (old_proc->main_thread) {
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG,
               "Call thread_fork() to get a new main thread for the fork.\n");
#endif
        pthread_t new_tid = thread_fork(new_proc->pid);
        if (new_tid < 0) {
#ifdef configPROC_DEBUG
            KERROR(KERROR_DEBUG, "thread_fork() failed\n");
#endif
            retval = -EAGAIN;
            goto out;
        } else if (new_tid > 0) { /* thread of the forking process returning */
#ifdef configPROC_DEBUG
            KERROR(KERROR_DEBUG, "\tthread_fork() fork OK\n");
#endif
            new_proc->main_thread = thread_lookup(new_tid);
            new_proc->main_thread->pid_owner = new_proc->pid;
            new_proc->main_thread->curr_mpt = &new_proc->mm.mpt;
        } else {
            /*
             * This should never happen but in case it still happens
             * we just exit with an error code.
             */
            new_proc->main_thread = NULL;
            new_proc->state = PROC_STATE_ZOMBIE;
            retval = -EAGAIN;
        }
    } else {
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "No main thread to fork.\n");
#endif
        new_proc->main_thread = NULL;
    }
    retval = new_proc->pid;

    new_proc->state = PROC_STATE_READY;

#ifdef configPROCFS
    procfs_mkentry(new_proc);
#endif

    if (new_proc->main_thread) {
#ifdef configPROC_DEBUG
        KERROR(KERROR_DEBUG, "Run new_proc->main_thread\n");
#endif
        thread_ready(new_proc->main_thread->id);
    }

#ifdef configPROC_DEBUG
    KERROR(KERROR_DEBUG, "Fork created.\n");
#endif

out:
    if (unlikely(retval < 0)) {
        _proc_free(new_proc);
    }
    return retval;
}
