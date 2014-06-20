/**
 *******************************************************************************
 * @file    proc.h
 * @author  Olli Vanhoja
 * @brief   Kernel process management header file.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2014 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
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

/** @addtogroup proc
 * Process Management.
 * Regions
 * -------
 *  Code region must be allocated globally and stored separately from processes
 *  allow processes to share the same code without copying its dynmem area on
 *  copy-on-write.
 *
 *  Stack and heap can be allocated as a single 1 MB dynmem allocation split in
 *  three sections on their own page table.
 *
 *  When region is freed its page tables must be also freed.
 * @{
 */

#pragma once
#ifndef PROC_H
#define PROC_H

#include <autoconf.h>

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <kernel.h>
#include <sched.h> /* Needed for threadInfo_t and threading functions */
#include <hal/mmu.h>
#include <vm/vm.h>
#include <fs/fs.h>
#include <klocks.h>
#include <sys/resource.h>

#define PROC_STATE_INITIAL  0
#define PROC_STATE_RUNNING  1
#define PROC_STATE_READY    2   /* Can be woken up, ready to run */
#define PROC_STATE_WAITING  3   /* Can't be woken up */
#define PROC_STATE_ZOMBIE   4
#define PROC_STATE_STOPPED  8

#define PROC_NAME_LEN       10


/**
 * Process Control Block.
 */
typedef struct proc_info {
    pid_t pid;
    char name[PROC_NAME_LEN]; /*!< process name */
    int state;                  /*!< 0 - running, >0 stopped */
    int priority;               /*!< We might want to prioritize processes too */
    long counter;               /*!< Counter for process running time */
    unsigned long blocked;      /*!< bitmap of masked signals */
    int exit_code, exit_signal;
    uid_t uid, euid, suid, fsuid;
    gid_t gid, egid, sgid, fsgid;
    unsigned long timeout;      /*!< Used to kill processes with absolute timeout */
    long utime, stime, cutime, cstime, start_time; /*!< For performance statistics */
    struct rlimit rlim; /*!< hard and soft limit for filesize TODO: own struct or just pointer? */

    /* open file information */
    struct vnode * croot;       /*!< Current root dir. */
    struct vnode * cwd;         /*!< Current working dir. */
    files_t * files;            /*!< Open files */
    struct tty_struct * tty;    /* NULL if no tty */

    /* Memory Management */
    struct vm_mm_struct mm;
    void * brk_start;           /*!< Break start address. (end of heap data) */
    void * brk_stop;            /*!< Break stop address. (end of heap region) */

    /* notes:
     * - main_thread already has a linked list of child threads
     * - file_t fd's
     * - tty
     * - etc.
     */

    /**
     * Process inheritance; Parent and child thread pointers.
     * inh : Parent and child process relations
     * ----------------------------------------
     * + first_child is a parent process attribute containing address to a first
     *   child of the parent process
     * + parent is a child process attribute containing address of the parent
     *   process of the child thread
     * + next_child is a child thread attribute containing address of a next
     *   child node of the common parent thread
     */
    struct inh {
        struct proc_info * parent;      /*!< Parent thread. */
        struct proc_info * first_child; /*!< Link to the first child. */
        struct proc_info * next_child;  /*!< Next child of the common parent. */
    } inh;

    threadInfo_t * main_thread; /*!< Main thread of this process. */
    /* signal handlers */
    struct signals sigs;        /*!< Signals. */
} proc_info_t;

/* Proc dab handling error codes */
#define PROC_DABERR_NOPROC  1
#define PROC_DABERR_INVALID 2
#define PROC_DABERR_PROT    3
#define PROC_DABERR_ENOMEM  4

extern int maxproc;                 /*!< Maximum # of processes, set. */
extern int act_maxproc;             /*!< Effective maxproc. */
extern int nprocs;                  /*!< Current # of procs. */
extern pid_t current_process_id;    /*!< PID of current process. */
extern proc_info_t * curproc;       /*!< PCB of the current process. */

/* proclock - Protects proc array, data structures and variables in proc. */
/** proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
extern mtx_t proclock;
#define PROC_LOCK()         mtx_spinlock(&proclock)
#define PROC_UNLOCK()       mtx_unlock(&proclock)
#define PROC_TESTLOCK()     mtx_test(&proclock)
#define PROC_LOCK_INIT()    mtx_init(&proclock, MTX_DEF | MTX_SPIN)

/**
 * Init process handling subsystem.
 */
void proc_init(void) __attribute__((constructor));

/* proc.c
 * Process scheduling and sys level management
 */

/**
 * Remove thread from a process.
 * @param pid       is a process id of the thread owner process.
 * @param thread_id is a thread if of the removed thread.
 */
void proc_thread_removed(pid_t pid, pthread_t thread_id);

/**
 * A process enters kernel mode.
 * This function shall be called when a process enters kernel mode and
 * interrupts will be enabled.
 * @return Next master page table.
 */
mmu_pagetable_t * proc_enter_kernel(void);

/**
 * A process exits kernel mode.
 * This function shall be called when a process exits kernel mode.
 * @return Next master page table.
 */
mmu_pagetable_t * proc_exit_kernel(void);

/**
 * Suspend the current process.
 */
void proc_suspend(void);

/**
 * Resume the current process.
 * A process is begin resumed from suspended state.
 * @return Next master page table.
 */
mmu_pagetable_t * proc_resume(void);

/**
 * Handle page fault caused by a process.
 * Usually this handler is executed because of cow page table.
 */
int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        threadInfo_t * thread);

/**
 * Update process system state.
 * Updates current_process_id and curproc.
 * @note This function is called by interrupt handler(s).
 */
pid_t proc_update(void);

/**
 * Replace the image of a given process with a new one.
 * The new image must be mapped in kernel memory space.
 * @param pid   Process id.
 * @param image Process image to be loaded.
 * @param size  Size of the image.
 * @return  Value other than zero if unable to replace process.
 */
int proc_replace(pid_t pid, void * image, size_t size);

void _proc_free(proc_info_t * p);

/**
 * Get pointer to a internal proc_info structure.
 */
proc_info_t * proc_get_struct(pid_t pid);

/* proc_fork.c */

/**
 * Create a new process.
 * @param pid   Process id.
 * @return  New PID; 0 if returning fork; -1 if unable to fork.
 */
pid_t proc_fork(pid_t pid);

#endif /* PROC_H */

/**
 * @}
 */

