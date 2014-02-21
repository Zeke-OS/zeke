/**
 *******************************************************************************
 * @file    proc.c
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
#include <proc.h>

static proc_info_t ** _procarr;
int maxproc = configMAXPROC;        /*!< Maximum # of processes. */
static int _cur_maxproc;
int nprocs;                         /* Current # of procs. */
volatile pid_t current_process_id;  /*!< PID of current process. */
volatile proc_info_t * curproc;     /*!< PCB of the current process. */
static pid_t _lastpid;              /*!< last allocated pid. */

#define SIZEOF_PROCARR      ((maxproc + 1) * sizeof(proc_info_t *))

#if configMP != 0
static mtx_t proclock;
#define PROCARR_LOCK()      mtx_spinlock(&proclock)
#define PROCARR_UNLOCK()    mtx_unlock(&proclock)
#define PROCARR_LOCK_INIT() mtx_init(&proclock, MTX_DEF | MTX_SPIN)
#else /* No MP support */
#define PROCARR_LOCK()
#define PROCARR_UNLOCK()
#define PROCARR_LOCK_INIT()
#endif

SYSCTL_INT(_kern, KERN_MAXPROC, maxproc, CTLFLAG_RWTUN,
    &maxproc, 0, "Maximum number of processes");
SYSCTL_INT(_kern, KERN_MAXPROC, nprocs, CTLFLAG_RD,
    &nprocs, 0, "Current number of processes");

static void realloc__procarr(void);
static pid_t get_random_pid(void);
static void set_proc_inher(proc_info_t * old_proc, proc_info_t * new_proc);

/**
 * Init process handling subsystem.
 */
void proc_init(void) __attribute__((constructor));
void proc_init(void)
{
    SUBSYS_INIT();
    PROCARR_LOCK_INIT();
    realloc__procarr();
    memset(&_procarr, 0, SIZEOF_PROCARR);
    SUBSYS_INITFINI("Proc init");
}

static void realloc__procarr(void)
{
    proc_info_t ** tmp;

    PROCARR_LOCK();
    tmp = krealloc(_procarr, SIZEOF_PROCARR);
    if ((tmp == 0) && (_procarr == 0)) {
        char buf[80];
        ksprintf(buf, sizeof(buf), "Unable to allocate _procarr (%u bytes)", SIZEOF_PROCARR);
        panic(buf);
    }
    _procarr = tmp;
    _cur_maxproc = maxproc;
    PROCARR_UNLOCK();
}

#if 0
/**
 * Initialize a new process.
 * @param image Process image to be loaded.
 * @param size  Size of the image.
 * @return  PID; -1 if unable to initialize.
 */
pid_t process_init(void * image, size_t size)
{
    return -1;
}
#endif

/**
 * Create a new process.
 * @param pid   Process id.
 * @return  New PID; -1 if unable to fork.
 */
pid_t proc_fork(pid_t pid)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html
     */

    proc_info_t * old_proc;
    proc_info_t * new_proc;
    pid_t retval = -EAGAIN;

    realloc__procarr();

    old_proc = proc_get_struct(pid);
    if (!old_proc) {
        retval = -EINVAL;
        goto out;
    }

    /* Allocate a PCB. */
    new_proc = kmalloc(sizeof(proc_info_t));
    if (!new_proc) {
        retval = -ENOMEM;
        goto out;
    }

    /* Allocate a master page table for the new process. */
    if(ptmapper_alloc(&(new_proc->mm.mptable))) {
        retval = -ENOMEM;
        goto free_new_proc;
    }

    /* Allocate an array for regions. */
    new_proc->mm.regions =
        kmalloc(old_proc->mm.nr_regions * sizeof(mmu_region_t *));
    if(!new_proc->mm.regions) {
        retval = -ENOMEM;
        goto free_pptable_arr;
    }
    new_proc->mm.nr_regions = old_proc->mm.nr_regions;

    /* Clone master page table. */
    if(mmu_ptcpy(&(new_proc->mm.mptable), &(new_proc->mm.mptable))) {
        retval = -EINVAL;
        goto free_regions_arr;
    }

    /* TODO Allocate othet page tables */

    /* Mark pages as rw and do lazy copy/copy on write later. */

    new_proc->pid = get_random_pid();

    /* A process shall be created with a single thread. If a multi-threaded
     * process calls fork(), the new process shall contain a replica of the
     * calling thread.
     */
    /* TODO make a clone of current_thread */
    //new_proc->main_thread = (threadInfo_t *)current_thread;

    /* Update inheritance attributes */
    set_proc_inher(old_proc, new_proc);

    /* TODO Insert to proc data structure */

    goto out; /* Fork created. */
free_regions_arr:
    kfree(new_proc->mm.regions);
free_pptable_arr:
    ptmapper_free(&(new_proc->mm.mptable));
free_new_proc:
    kfree(new_proc);
out:
    return retval;
}

/**
 * Get a random PID for a new process.
 * @return Returns a random PID.
 */
static pid_t get_random_pid(void)
{
    pid_t newpid = maxproc + 1;

    /* The new PID will be "randomly" selected between _lastpid and maxproc */
    do {
        if (newpid > maxproc)
            newpid = _lastpid + kunirand(maxproc - _lastpid - 1) + 1;
        newpid++;
    } while (proc_get_struct(newpid));

    return newpid;
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

int proc_kill(void)
{
    return -1;
}

/**
 * Replace the image of a given process with a new one.
 * The new image must be mapped in kernel memory space.
 * @param pid   Process id.
 * @param image Process image to be loaded.
 * @param size  Size of the image.
 * @return  Value other than zero if unable to replace process.
 */
int proc_replace(pid_t pid, void * image, size_t size)
{
    return -1;
}

proc_info_t * proc_get_struct(pid_t pid)
{
    return (proc_info_t *)0;
}

/**
 * Remove thread from a process.
 * @param pid       is a process id of the thread owner process.
 * @param thread_id is a thread if of the removed thread.
 */
void proc_thread_removed(pid_t pid, pthread_t thread_id)
{
    /* TODO
     * + Free stack area
     * + Remove thread pointer
     */
}

/**
 * Get page table descriptor of a process.
 * @param pid Process ID.
 * @return Page table descriptor.
 */
mmu_pagetable_t * proc_get_pptable(pid_t pid)
{
    mmu_pagetable_t * pptable;

    if (pid == 0) { /* Kernel master page table requested. */
        pptable = &mmu_pagetable_master;
    } else { /* Get the process master page table */
        pptable = &(proc_get_struct(pid)->mm.mptable);
    }

    return pptable;
}

/**
 * Try to handler page fault caused by a process.
 * Usually this handler is executed because of cow page table.
 * @param pid  is a process id of the process that caused a page fault.
 * @param vaddr is the page faulted address.
 */
int proc_cow_handler(pid_t pid, void * vaddr)
{
    /* TODO
     * - lookup for region
     * - if region found, clone dynmem and fix possible region groups spanning
     *   over dynmem region
     * - else return -1
     *
     *   Clone: if (region->reg_vm.usr_rw == VM_PROT_COW)
     *              region->reg_vm.rclone(region->paddr);
     */

    return -1;
}

/**
 * Update process system state.
 * Updates current_process_id and curproc.
 * @note This function is called by interrupt handler(s).
 */
void proc_update(void)
{
    current_process_id = current_thread->pid_owner;
    proc_get_struct(current_process_id);
}

uintptr_t proc_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_PROC_EXEC: /* note: can only return EAGAIN or ENOMEM */
        current_thread->errno = ENOSYS;
        return -1;

    case SYSCALL_PROC_FORK:
        current_thread->errno = ENOSYS;
        return -2;

    case SYSCALL_PROC_WAIT:
        current_thread->errno = ENOSYS;
        return -3;

    case SYSCALL_PROC_EXIT:
        current_thread->errno = ENOSYS;
        return -4;

    case SYSCALL_PROC_GETUID:
        current_thread->errno = ENOSYS;
        return -5;

    case SYSCALL_PROC_GETEUID:
        current_thread->errno = ENOSYS;
        return -6;

    case SYSCALL_PROC_GETGID:
        current_thread->errno = ENOSYS;
        return -7;

    case SYSCALL_PROC_GETEGID:
        current_thread->errno = ENOSYS;
        return -8;

    case SYSCALL_PROC_GETPID:
        current_thread->errno = ENOSYS;
        return -9;

    case SYSCALL_PROC_GETPPID:
        current_thread->errno = ENOSYS;
        return -10;

    case SYSCALL_PROC_ALARM:
        current_thread->errno = ENOSYS;
        return -13;

    case SYSCALL_PROC_CHDIR:
        current_thread->errno = ENOSYS;
        return -14;

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */
