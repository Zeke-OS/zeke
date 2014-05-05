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

/** @addtogroup Process
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

/* Process Management Notes
 * ========================
 *
 * Regions
 * -------
 * Code region must be allocated globally and stored separately from processes
 * allow processes to share the same code without copying its dynmem area on
 * copy-on-write.
 *
 * Stack & heap can be allocated as a single 1 MB dynmem allocation split in
 * three sections on their own page table.
 *
 * When region is freed its page tables must be also freed.
 */

/**
 * Process Control Block or Process Descriptor Structure
 */

#define PROC_STATE_INITIAL  0
#define PROC_STATE_RUNNING  1
#define PROC_STATE_RUNNABLE 2   /* Can be woken up, ready to run */
#define PROC_STATE_WAITING  3   /* Can't be woken up */
#define PROC_STATE_ZOMBIE   4
#define PROC_STATE_STOPPED  8

#define PROC_NAME_LEN       10


/**
 * Process Control Block.
 */
typedef struct {
    pid_t pid;
    char name[PROC_NAME_LEN]; /*!< process name */
    int state; /*!< 0 - running, >0 stopped */
    int priority; /*!< We might want to prioritize processes too */
    long counter; /*!< Counter for process running time */
    unsigned long blocked; /*!< bitmap of masked signals */
    int exit_code, exit_signal;
    uid_t uid, euid, suid, fsuid;
    gid_t gid, egid, sgid, fsgid;
    unsigned long timeout; /*!< Used to kill processes with absolute timeout */
    long utime, stime, cutime, cstime, start_time; /*!< For performance statistics */
    struct rlimit rlim; /*!< hard and soft limit for filesize TODO: own struct or just pointer? */
    /* open file information */
    struct vnode * cwd; /*!< Current working dir. */
    files_t * files; /*!< Open files */
    struct tty_struct * tty; /* NULL if no tty */

    /* Memory Management */
    struct vm_mm_struct mm;
    void * brk;
    void * brk_start;
    void * brk_stop;

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
        void * parent;      /*!< Parent thread. */
        void * first_child; /*!< Link to the first child thread. */
        void * next_child;  /*!< Next child of the common parent. */
    } inh;

    threadInfo_t * main_thread; /*!< Main thread of this process. */
    /* signal handlers */
    sigs_t sigs;                /*!< Signals. */
} proc_info_t;

extern int maxproc;
extern int nprocs;
extern pid_t current_process_id;
extern proc_info_t * curproc;

void proc_init(void) __attribute__((constructor));

/* Functions that are more or less internal to proc subsys */
void procarr_realloc(void);
void procarr_insert(proc_info_t * new_proc);
pid_t proc_get_random_pid(void);

pid_t proc_fork(pid_t pid);
int proc_kill(void);
int proc_replace(pid_t pid, void * image, size_t size);
void proc_thread_removed(pid_t pid, pthread_t thread_id);
proc_info_t * proc_get_struct(pid_t pid);
mmu_pagetable_t * pr_get_mptable(pid_t pid);
int proc_dab_handler(pid_t pid, intptr_t vaddr);

#endif /* PROC_H */

/**
  * @}
  */

