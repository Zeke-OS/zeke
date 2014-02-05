/**
 *******************************************************************************
 * @file    process.c
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
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
#include <vm/vm.h>
#include <ptmapper.h>
#include <kerror.h>
#include <kmalloc.h>
#include <process.h>

static proc_info_t ** _procarr;
int maxproc = configMAXPROC; /*!< Maximum number of processes. */
static int _cur_maxproc;
volatile pid_t current_process_id; /*!< PID of current process. */
volatile proc_info_t * curproc; /*!< Pointer to the PCB of current process. */

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

static void realloc__procarr(void);
static void set_process_inher(proc_info_t * old_proc, proc_info_t * new_proc);

/**
 * Init process handling subsystem.
 */
void process_init(void) __attribute__((constructor));
void process_init(void)
{
    PROCARR_LOCK_INIT();
    realloc__procarr();
}

static void realloc__procarr(void)
{
    proc_info_t ** tmp;

    PROCARR_LOCK();
    tmp = krealloc(_procarr, maxproc);
    if ((tmp == 0) && (_procarr == 0))
        panic("Unable to allocate _procarr.");
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
pid_t process_fork(pid_t pid)
{
    /*
     * http://pubs.opengroup.org/onlinepubs/9699919799/functions/fork.html
     */

    proc_info_t * old_proc;
    proc_info_t * new_proc;
    pid_t retval = -EAGAIN;

    realloc__procarr();

    old_proc = process_get_struct(pid);
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

    /* Allocate the actual page table. */
    if(ptmapper_alloc(&(new_proc->pptable))) {
        retval = -ENOMEM;
        goto free_new_proc;
    }

    /* Clone page table. */
    if(mmu_clone_pt(&(new_proc->pptable), &(new_proc->pptable))) {
        retval = -EINVAL;
        goto free_pptable;
    }

    /* A process shall be created with a single thread. If a multi-threaded
     * process calls fork(), the new process shall contain a replica of the
     * calling thread
     */
    new_proc->main_thread = (threadInfo_t *)current_thread;

    /* Update inheritance attributes */
    set_process_inher(old_proc, new_proc);

    /* TODO Insert to proc data structure */

    goto out; /* Fork created. */
free_pptable:
    ptmapper_free(&(new_proc->pptable));
free_new_proc:
    kfree(new_proc);
out:
    return retval;
}

static void set_process_inher(proc_info_t * old_proc, proc_info_t * new_proc)
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

int process_kill(void)
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
int process_replace(pid_t pid, void * image, size_t size)
{
    return -1;
}

proc_info_t * process_get_struct(pid_t pid)
{
    return (proc_info_t *)0;
}

/**
 * Get page table descriptor of a process.
 * @param pid Process ID.
 * @return Page table descriptor.
 */
mmu_pagetable_t * process_get_pptable(pid_t pid)
{
    mmu_pagetable_t * pptable;

    if (pid == 0) { /* Kernel master page table requested. */
        pptable = &mmu_pagetable_master;
    } else { /* Get the process master page table */
        pptable = &(process_get_struct(pid)->pptable);
    }

    return pptable;
}

/**
 * Update process system state.
 * @note Updates current_process_id and curproc.
 */
void process_update(void)
{
    current_process_id = current_thread->pid_owner;
    /* TODO Updated curproc */
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
