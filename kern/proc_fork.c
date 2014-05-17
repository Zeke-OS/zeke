/**
 *******************************************************************************
 * @file    proc_fork.c
 * @author  Olli Vanhoja
 * @brief   Kernel process management source file. This file is responsible for
 *          thread creation and management.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup Process
  * @{
  */

#define KERNEL_INTERNAL 1
#include <sched.h>
#include <kstring.h>
#include <libkern.h>
#include <kerror.h>
#include <kinit.h>
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
#include <vm/vm.h>
#include <sys/sysctl.h>
#include <ptmapper.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <vralloc.h>
#include <proc.h>

static int clone_L2_pt(proc_info_t * const new_proc, proc_info_t * const old_proc);
static void set_proc_inher(proc_info_t * old_proc, proc_info_t * new_proc);

/**
 * Create a new process.
 * @param pid   Process id.
 * @return  New PID; 0 if returning fork; -1 if unable to fork.
 */
pid_t proc_fork(pid_t pid)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html
     */
#if configDEBUG >= KERROR_DEBUG
    {
    char buf[40];
    ksprintf(buf, sizeof(buf), "fork(%u)", pid);
    KERROR(KERROR_DEBUG, buf);
    }
#endif

    proc_info_t * const old_proc = proc_get_struct(pid);
    proc_info_t * const new_proc = kmalloc(sizeof(proc_info_t));
    pid_t retval = -EAGAIN;

    procarr_realloc();

    /* Check that the old PID was valid. */
    if (!old_proc || (old_proc->state == PROC_STATE_INITIAL)) {
        retval = -EINVAL;
        goto out;
    }

    /* Check that a new PCB was allocated. */
    if (!new_proc) {
        retval = -ENOMEM;
        goto out;
    }

    /* Allocate a master page table for the new process. */
    new_proc->mm.mpt.vaddr = 0;
    new_proc->mm.mpt.type = MMU_PTT_MASTER;
    new_proc->mm.mpt.dom = MMU_DOM_USER;
    if (ptmapper_alloc(&(new_proc->mm.mpt))) {
        retval = -ENOMEM;
        goto free_new_proc;
    }
    new_proc->mm.curr_mpt = &new_proc->mm.mpt;

    /* Allocate an array for regions. */
    new_proc->mm.regions =
        kmalloc(old_proc->mm.nr_regions * sizeof(vm_region_t *));
    if (!new_proc->mm.regions) {
        retval = -ENOMEM;
        goto free_pptable_arr;
    }
    new_proc->mm.nr_regions = old_proc->mm.nr_regions;

    /* Clone master page table. */
    if (mmu_ptcpy(&(new_proc->mm.mpt), &(old_proc->mm.mpt))) {
        retval = -EAGAIN; // Actually more like -EINVAL
        goto free_regions_arr;
    }

    /* Clone L2 page tables. */
    if (clone_L2_pt(new_proc, old_proc) < 0) {
        goto free_vpt_rb;
    }

    /* Variables for cloning and referencing regions. */
    struct vm_pt * vpt;
    vm_region_t * vm_reg_tmp;

    /* Copy code region pointer. */
    vm_reg_tmp = (*old_proc->mm.regions)[MM_CODE_REGION];
    if (!vm_reg_tmp) {
        panic("Old proc code region can't be null");
    }
    if (vm_reg_tmp->vm_ops)
        vm_reg_tmp->vm_ops->rref(vm_reg_tmp);
    (*new_proc->mm.regions)[MM_CODE_REGION] = vm_reg_tmp;

    /* Clone stack region. */
    vm_reg_tmp = (*old_proc->mm.regions)[MM_STACK_REGION];
    if (vm_reg_tmp && vm_reg_tmp->vm_ops) { /* Only if vmp_ops are defined. */
#if configDEBUG >= KERROR_DEBUG
        KERROR(KERROR_DEBUG, "Cloning stack");
#endif
        if (!vm_reg_tmp->vm_ops->rclone)
            panic("No clone operation");

        vm_reg_tmp = vm_reg_tmp->vm_ops->rclone(vm_reg_tmp);
        if (!vm_reg_tmp) {
            retval = -ENOMEM;
            goto free_regions;
        }
    } else if (vm_reg_tmp) { /* Try to clone the stack manually. */
#if configDEBUG >= KERROR_DEBUG
        KERROR(KERROR_DEBUG, "Cloning stack manually");
#endif
        vm_region_t * old_region = vm_reg_tmp;
        size_t rsize = MMU_SIZEOF_REGION(&(vm_reg_tmp->mmu));

        vm_reg_tmp = vralloc(rsize);
        if (!vm_reg_tmp)
            panic("OOM during fork()");

        memcpy((void *)(vm_reg_tmp->mmu.paddr), (void *)(old_region->mmu.paddr),
                rsize);
        vm_reg_tmp->usr_rw = VM_PROT_READ | VM_PROT_WRITE;
        vm_reg_tmp->mmu.vaddr = old_region->mmu.vaddr;
        vm_reg_tmp->mmu.ap = old_region->mmu.ap;
        vm_reg_tmp->mmu.control = old_region->mmu.control;
        /* paddr already set */
        vm_reg_tmp->mmu.pt = old_region->mmu.pt;
        vm_updateusr_ap(vm_reg_tmp);
    } else { /* else: NO STACK */
#if configDEBUG >= KERROR_DEBUG
        KERROR(KERROR_DEBUG, "fork: No stack created");
#endif
    }

    if (vm_reg_tmp) {
        if ((vpt = ptlist_get_pt(
                &(new_proc->mm.ptlist_head),
                &(new_proc->mm.mpt),
                vm_reg_tmp->mmu.vaddr)) == 0) {
            retval = -ENOMEM;
            goto free_regions;
        }
        (*new_proc->mm.regions)[MM_STACK_REGION] = vm_reg_tmp;

        vm_map_region((*new_proc->mm.regions)[MM_STACK_REGION], vpt);
    }

    /* Copy other region pointers.
     * As an iteresting sidenote, what we are doing here and earlier when L1
     * page table was cloned is that we are losing link between the region
     * structs and the actual L1 page table of this process. However it
     * doesn't matter at all because we are doing COW anyway so no information
     * is ever completely lost but we have to just keep in mind that COW regions
     * are bit incomplete and L1 is not completely reconstructable by using just
     * vm_region struct.
     *
     * Many BSD variants have fully reconstructable L1 tables but we don't have
     * it directly that way because shared regions can't properly point to more
     * than one page table struct.
     */
    for (int i = MM_HEAP_REGION; i < old_proc->mm.nr_regions; i++) {
        vm_reg_tmp = (*old_proc->mm.regions)[i];
        if (vm_reg_tmp->vm_ops && vm_reg_tmp->vm_ops->rref)
            vm_reg_tmp->vm_ops->rref(vm_reg_tmp);

        (*new_proc->mm.regions)[i] = vm_reg_tmp;

        /* Skip for regions in system page table */
        if ((*new_proc->mm.regions)[i]->mmu.vaddr <= MMU_VADDR_KERNEL_END)
            continue;

        /* Set COW bit */
        if ((*new_proc->mm.regions)[i]->usr_rw & VM_PROT_WRITE) {
            (*new_proc->mm.regions)[i]->usr_rw |= VM_PROT_COW;
#if 0
            /* Unnecessary as vm_map_region() will do this too. */
            vm_updateusr_ap((*new_proc->mm.regions)[i]);
#endif
        }

        vpt = ptlist_get_pt(
                &(new_proc->mm.ptlist_head),
                &(new_proc->mm.mpt),
                (*new_proc->mm.regions)[i]->mmu.vaddr);
        if (vpt == 0) {
            retval = -ENOMEM;
            goto free_regions;
        }

        /* Attach region with page table owned by the new process. */
        vm_map_region((*new_proc->mm.regions)[i], vpt);
    }

    /* Break values */
    new_proc->brk_start = old_proc->brk_start;
    new_proc->brk_stop = old_proc->brk_stop;

    /* Copy file descriptors */
    new_proc->files = kmalloc(SIZEOF_FILES(old_proc->files->count));
    if (!new_proc->files) {
        retval = -ENOMEM;
        goto free_regions;
    }
    new_proc->files->count = old_proc->files->count;
    for (int i = 0; i < old_proc->files->count; i++) {
        new_proc->files->fd[i] = old_proc->files->fd[i];
        if (new_proc->files->fd[i]) /* TODO Lock? */
            new_proc->files->fd[i]->refcount++;
    }

    if (nprocs != 1) { /* Tecnically it would be good idea to have lock on
                        * nprocs before reading it but I think this should
                        * work fine... */
        new_proc->pid = proc_get_random_pid();
    } else { /* Proc is init */
        new_proc->pid = 1;
    }

    /* A process shall be created with a single thread. If a multi-threaded
     * process calls fork(), the new process shall contain a replica of the
     * calling thread.
     * We left main_thread null if calling process has no main thread.
     */
    if (old_proc->main_thread) {
        pthread_t old_tid = get_current_tid();
        pthread_t new_tid = sched_thread_fork();
        if (new_tid < 0) {
            retval = -EAGAIN; /* TODO ?? */
            goto free_files;
        } else if (new_tid > 0) { /* thread of the forking process returning */
            new_proc->main_thread = sched_get_pThreadInfo(new_tid);
            new_proc->main_thread->pid_owner = new_proc->pid;
        } else { /* 0, new thread returning */
            panic("XXX"); /* TODO Remove */
            retval = 0;
            goto out;
        }
    } else {
        new_proc->main_thread = 0;
    }
    retval = new_proc->pid;

    /* Update inheritance attributes */
    set_proc_inher(old_proc, new_proc);

    /* TODO state */
    new_proc->state = PROC_STATE_STOPPED;

    /* Insert the new process to _procarr */
    procarr_insert(new_proc);

    if (new_proc->main_thread) {
        /* TODO Should be enabled */
        sched_thread_set_exec(new_proc->main_thread->id);
    }
#if configDEBUG >= KERROR_DEBUG
    {
        char buf[80];
        ksprintf(buf, sizeof(buf), "Fork created: %u", get_current_tid());
        KERROR(KERROR_DEBUG, buf);
    }
#endif
    goto out; /* Fork created. */

free_files:
    /* TODO */
free_regions:
    for (int i = 0; i < new_proc->mm.nr_regions; i++) {
        if ((*new_proc->mm.regions)[i]->vm_ops->rfree)
            (*new_proc->mm.regions)[i]->vm_ops->rfree((*new_proc->mm.regions)[i]);
    }
    new_proc->mm.nr_regions = 0;
free_vpt_rb:
    ptlist_free(&(new_proc->mm.ptlist_head));
free_regions_arr:
    kfree(new_proc->mm.regions);
free_pptable_arr:
    ptmapper_free(&(new_proc->mm.mpt));
free_new_proc:
    kfree(new_proc);
out:
    return retval;
}

/**
 * Clone L2 page tables of a process.
 * @param new_proc is the target process.
 * @param old_proc is a the source process.
 * @return  Returns positive value indicating number of copied page tables;
 *          Zero idicating that no page tables were copied;
 *          Negative value idicating that copying page tables failed.
 */
static int clone_L2_pt(proc_info_t * const new_proc, proc_info_t * const old_proc)
{
    struct vm_pt * old_vpt;
    struct vm_pt * new_vpt;
    int retval = 0;

    /* Clone L2 page tables. */
    RB_INIT(&(new_proc->mm.ptlist_head));
    if (RB_EMPTY(&(old_proc->mm.ptlist_head))) {
        goto out;
    }
    RB_FOREACH(old_vpt, ptlist, &(old_proc->mm.ptlist_head)) {
        /* TODO for some reason linkcount might be invalid! */
#if 0
        if (old_vpt->linkcount <= 0) {
            continue; /* Skip unused page tables; ie. page tables that are
                       * not referenced by any region. */
        }
#endif

        new_vpt = kmalloc(sizeof(struct vm_pt));
        if (!new_vpt) {
            retval = -ENOMEM;
            goto out;
        }

        new_vpt->linkcount = 1;
        new_vpt->pt.vaddr = old_vpt->pt.vaddr;
        new_vpt->pt.master_pt_addr = new_proc->mm.mpt.pt_addr;
        new_vpt->pt.type = MMU_PTT_COARSE;
        new_vpt->pt.dom = old_vpt->pt.dom;

        /* Allocate the actual page table, this will also set pt_addr. */
        if(ptmapper_alloc(&(new_vpt->pt))) {
            retval = -ENOMEM;
            goto out;
        }

        mmu_ptcpy(&(new_vpt->pt), &(old_vpt->pt));

        /* Insert vpt (L2 page table) to the new new process. */
        RB_INSERT(ptlist, &(new_proc->mm.ptlist_head), new_vpt);
        mmu_attach_pagetable(&(new_vpt->pt));

        retval++; /* Increment vpt copied count. */
    }

out:
    return retval;
}

static void set_proc_inher(proc_info_t * old_proc, proc_info_t * new_proc)
{
    proc_info_t * last_node, * tmp;

    /* Initial values */
    new_proc->inh.parent = old_proc;
    new_proc->inh.first_child = NULL;
    new_proc->inh.next_child = NULL;

    if (old_proc->inh.first_child == NULL) {
        /* This is the first child of this parent */
        old_proc->inh.first_child = new_proc;
        new_proc->inh.next_child = NULL;

        return; /* All done */
    }

    /* Find the last child process
     * Assuming first_child is a valid pointer
     */
    tmp = (proc_info_t *)(old_proc->inh.first_child);
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly forked thread as the last child in chain. */
    last_node->inh.next_child = new_proc;
}

/**
  * @}
  */
