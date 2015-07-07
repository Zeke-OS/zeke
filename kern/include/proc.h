/**
 *******************************************************************************
 * @file    proc.h
 * @author  Olli Vanhoja
 * @brief   Kernel process management header file.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup proc
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

#include <sys/types.h>
#include <sys/types_pthread.h>
#include <sys/param.h>
#include <sys/priv.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <bitmap.h>
#include <fs/fs.h>
#include <hal/mmu.h>
#include <klocks.h>
#include <thread.h>
#include <vm/vm.h>

enum proc_state {
    PROC_STATE_INITIAL  = 0,
    #if 0
    PROC_STATE_RUNNING  = 1,
    #endif
    PROC_STATE_READY    = 2, /*!< Can be woken up, ready to run. */
    #if 0
    PROC_STATE_WAITING  = 3, /*!< Can't be woken up. */
    #endif
    PROC_STATE_STOPPED  = 4, /*!< Stopped with a signal SIGSTOP. */
    PROC_STATE_ZOMBIE   = 5,
    PROC_STATE_DEFUNCT  = 6  /*!< Process waiting for the final cleanup. */

};

#define PROC_NAME_LEN       16

struct thread_info;

/**
 * Process Control Block.
 */
struct proc_info {
    pid_t pid;
    char name[PROC_NAME_LEN];   /*!< Process name. */
    enum proc_state state;      /*!< Process state. */
    int priority;               /*!< We may want to prioritize processes too. */
    int exit_code, exit_signal;
    struct cred cred;           /*!< Process credentials. */

    /* Accounting */
    unsigned long timeout;          /*!< Absolute timeout of the process. */
    struct timespec * start_time;   /*!< For performance statistics. */
    struct tms tms;                 /*!< User, System and childred times. */
    struct rlimit rlim[_RLIMIT_ARR_COUNT]; /*!< Hard and soft limits. */

    /* Open file information */
    struct vnode * croot;       /*!< Current root dir. */
    struct vnode * cwd;         /*!< Current working dir. */
    files_t * files;            /*!< Open files */
    struct tty_struct * tty;    /* NULL if no tty */

    /* Memory Management */
    struct vm_mm_struct mm;
    void * brk_start;           /*!< Break start address. (end of heap data) */
    void * brk_stop;            /*!< Break stop address. (end of heap region) */

    /* Signals */
    struct signals sigs;        /*!< Per process signals. */
    uintptr_t usigret;          /*!< Address of the sigret() function in
                                 *   user space. */

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
        SLIST_HEAD(proc_child_list, proc_info) child_list_head;
        SLIST_ENTRY(proc_info) child_list_entry;
        mtx_t lock; /*!< Lock for children (child_list_entry) of this proc. */
    } inh;

    struct thread_info * main_thread; /*!< Main thread of this process. */
};

#define PROC_INH_LOCK_TYPE (MTX_TYPE_SPIN)

extern int maxproc;                 /*!< Maximum # of processes, set. */
extern int act_maxproc;             /*!< Effective maxproc. */
extern int nprocs;                  /*!< Current # of procs. */
extern struct proc_info * curproc;  /*!< PCB of the current process. */

/* proclock - Protects proc array, data structures and variables in proc. */
/** proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
extern mtx_t proclock;
#define PROC_LOCK()         mtx_lock(&proclock)
#define PROC_UNLOCK()       mtx_unlock(&proclock)
#define PROC_TESTLOCK()     mtx_test(&proclock)
#define PROC_LOCK_INIT()    mtx_init(&proclock, MTX_TYPE_SPIN, 0)

/*
 * proc.c
 * Process scheduling and sys level management
 */

/**
 * Iterate over threads owned by proc.
 * @param thread_it should be initialized to NULL.
 * @return next thread or NULL.
 */
struct thread_info * proc_iterate_threads(struct proc_info * proc,
        struct thread_info ** thread_it);

/**
 * Remove thread from a process.
 * @param pid       is a process id of the thread owner process.
 * @param thread_id is a thread if of the removed thread.
 */
void proc_thread_removed(pid_t pid, pthread_t thread_id);

void proc_update_times(void);

/**
 * Handle page fault caused by a process.
 * Usually this handler is executed because of cow page table.
 */
int proc_dab_handler(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
                     struct proc_info * proc, struct thread_info * thread);

/**
 * Update process system state.
 * Updates curproc.
 * @note This function is called by interrupt handler(s).
 */
pid_t proc_update(void);

/**
 * Free a process PCB and other related resources.
 */
void _proc_free(struct proc_info * p);

/**
 * Get pointer to a internal proc_info structure by first locking it.
 * Caller of this function should be the owner/parent of requested data or it
 * shall first disable interrupts to prevent removal of the data while using it.
 */
struct proc_info * proc_get_struct_l(pid_t pid);

/**
 * Get pointer to a internal proc_info structure.
 * @note Requires PROC_LOCK.
 */
struct proc_info * proc_get_struct(pid_t pid);

struct vm_mm_struct * proc_get_locked_mm(pid_t pid);

/* proc_fork.c */

/**
 * Create a new process.
 * @param pid   Process id.
 * @return  New PID; 0 if returning fork; -1 if unable to fork.
 */
pid_t proc_fork(pid_t pid);

#ifdef PROC_INTERNAL

/**
 * Realloc procarr.
 * Realloc _procarr based on maxproc sysctl variable if necessary.
 * @note    This should be generally called before selecting next pid
 *          from the array.
 */
void procarr_realloc(void);

/**
 * Insert a new process to _procarr.
 * @param proc is a pointer to the new process.
 */
void procarr_insert(struct proc_info * new_proc);

/**
 * Get a random PID for a new process.
 * @return Returns a random PID.
 */
pid_t proc_get_random_pid(void);

#endif /* PROC_INTERNAL */

#endif /* PROC_H */

/**
 * @}
 */

