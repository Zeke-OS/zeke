/**
 *******************************************************************************
 * @file    proc.h
 * @author  Olli Vanhoja
 * @brief   Kernel process management header file.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fs/fs.h>
#include <klocks.h>
#include <kobj.h>
#include <ksignal.h>
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

struct mmu_abo_param;
struct thread_info;

/**
 * Session.
 */
struct session {
    pid_t s_leader;             /*!< Session leader. */
    int s_ctty_fd;              /*!< fd number of the controlling terminal. */
    char s_login[MAXLOGNAME];   /*!< Setlogin() name. */
    struct kobj s_obj;
    TAILQ_HEAD(pgrp_list, pgrp) s_pgrp_list_head;
};

/**
 * Process group descriptor.
 */
struct pgrp {
    pid_t pg_id;                /*!< Pgrp id. */
    struct session * pg_session; /*!< Pointer to the session. */
    TAILQ_HEAD(proc_list, proc_info) pg_proc_list_head;
    struct kobj pg_obj;
    TAILQ_ENTRY(pgrp) pg_pgrp_entry_;
};

/**
 * Process Control Block.
 */
struct proc_info {
    pid_t pid;
    char name[PROC_NAME_LEN];   /*!< Process name. */
    enum proc_state state;      /*!< Process state. */
    int priority;               /*!< We may want to prioritize processes too. */
    int exit_code;
    struct ksiginfo * exit_ksiginfo; /*!< Set if killed with a signal. */
    struct pgrp * pgrp;         /*!< Process group. */
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
     */
    struct proc_inh {
        struct proc_info * parent;      /*!< A pointer to the parent process. */
        SLIST_HEAD(proc_child_list, proc_info) child_list_head;
        SLIST_ENTRY(proc_info) child_list_entry;
        mtx_t lock; /*!< Lock for children (child_list_entry) of this proc. */
    } inh;

    TAILQ_ENTRY(proc_info) pgrp_proc_entry_;

    struct thread_info * main_thread; /*!< Main thread of this process. */
};

/**
 * Get a pointer to the head of an inheritance list of a process.
 * @param _proc is a pointer to the process.
 */
#define PROC_INH_HEAD(_proc) (&(_proc)->inh.child_list_head)

/**
 * Test if a process doesn't have any children.
 * @param _proc is a pointer to the process.
 */
#define PROC_INH_IS_EMPTY(_proc) SLIST_EMPTY(PROC_INH_HEAD(_proc))

/**
 * Get a pointer to the first child of a process.
 * @param _proc is a pointer to the process.
 */
#define PROC_INH_FIRST(_proc) SLIST_FIRST(PROC_INH_HEAD(_proc))

#define PROC_INH_FOREACH(_var, _proc) \
    SLIST_FOREACH((_var), PROC_INH_HEAD(_proc), inh.child_list_entry)

#define PROC_INH_FOREACH_FROM(_var, _proc) \
    SLIST_FOREACH_FROM((_var), PROC_INH_HEAD(_proc), inh.child_list_entry)

#define PROC_INH_FOREACH_SAFE(_var, _tmp_var, _proc) \
    SLIST_FOREACH_SAFE((_var), PROC_INH_HEAD(_proc), inh.child_list_entry, \
                       (_tmp_var))

#define PROC_INH_FOREACH_FROM_SAFE(_var, _tmp_var, _proc) \
    SLIST_FOREACH_SAFE((_var), PROC_INH_HEAD(_proc), inh.child_list_entry, \
                       (_tmp_var))

#define PROC_INH_INIT(_proc) SLIST_INIT(PROC_INH_HEAD(_proc))

#define PROC_INH_INSERT_AFTER(_elm1, _elm2) \
    SLIST_INSERT_AFTER((_elm1), _elm2, inh.child_list_entry)

#define PROC_INH_INSERT_HEAD(_proc, _elm) \
    SLIST_INSERT_HEAD(PROC_INH_HEAD(_proc), (_elm), inh.child_list_entry)

#define PROC_INH_NEXT(_elm) \
    SLIST_NEXT((_elm), inh.child_list_entry)

#define PROC_INH_REMOVE_AFTER(_elm) \
    SLIST_REMOVE_AFTER((_elm), inh.child_list_entry)

#define PROC_INH_REMOVE_HEAD(_proc) \
    SLIST_REMOVE_HEAD(PROC_INH_HEAD(_proc), inh.child_list_entry)

#define PROC_INH_REMOVE(_proc, _elm) \
    SLIST_REMOVE(PROC_INH_HEAD(_proc), (_elm), proc_info, inh.child_list_entry)

#define PROC_INH_SWAP(_proc1, _proc2) \
    SLIST_SWAP(PROC_INH_HEAD(_proc1), PROC_INH_HEAD(_proc2), \
               inh.child_list_entry)

/**
 * Lock type used for a inheritance lsit synchronization.
 * @{
 */

/**
 * Type.
 */
#define PROC_INH_LOCK_TYPE (MTX_TYPE_SPIN)

/**
 * Options.
 */
#define PROC_INH_LOCK_OPT  (0)

/**
 * @}
 */

extern int nprocs;                  /*!< Current # of procs. */
extern struct proc_info * curproc;  /*!< PCB of the current process. */

/* proclock - Protects proc array, data structures and variables in proc. */
/**
 * proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
extern mtx_t proclock;
#define PROC_LOCK()         mtx_lock(&proclock)
#define PROC_UNLOCK()       mtx_unlock(&proclock)
#define PROC_TESTLOCK()     mtx_test(&proclock)

/**
 * Enum used by some functions to tell if the caller has locked proclock.
 */
enum proc_lock_mode {
    PROC_NOT_LOCKED,
    PROC_LOCKED,
};

/*
 * proc.c
 * Process scheduling and sys level management
 */

/**
 * Iterate over threads owned by proc.
 * @param thread_it should be initialized to NULL.
 * @return next thread or NULL.
 */
struct thread_info * proc_iterate_threads(const struct proc_info * proc,
                                          struct thread_info ** thread_it);

/**
 * Remove thread from a process.
 * @param pid       is a process id of the thread owner process.
 * @param thread_id is a thread if of the removed thread.
 */
void proc_thread_removed(pid_t pid, pthread_t thread_id);

/**
 * Handle page fault caused by a process.
 * Usually this handler is executed because of cow page table.
 */
int proc_abo_handler(const struct mmu_abo_param * restrict abo);

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
 * Test if process exists.
 */
int proc_exists(pid_t pid, enum proc_lock_mode lmode);

/**
 * Get a reference to a proc_info struct.
 */
struct proc_info * proc_ref(pid_t pid, enum proc_lock_mode lmode)
    __attribute__((warn_unused_result));

/**
 * Uref a proc.
 * @note Handles a NULL pointer.
 */
void proc_unref(struct proc_info * proc);

/**
 * Process state enum to string name of the state.
 */
const char * proc_state2str(enum proc_state state);

/* proc_fork.c */

/**
 * Create a new process by forking the current process.
 * @return  New PID; 0 if returning fork; -1 if unable to fork.
 */
pid_t proc_fork(void);

#ifdef PROC_INTERNAL

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

/**
 * @addtogroup session
 * Session management
 * @{
 */

static inline int proc_is_session_leader(struct proc_info * p)
{
    return (p->pid == p->pgrp->pg_session->s_leader);
}

#ifdef PROC_INTERNAL

/**
 * Remove a reference to the session s.
 */
void proc_session_remove(struct session * s);

void proc_session_setlogin(struct session * s, char s_login[MAXLOGNAME]);

/**
 * Search for a process group in a session.
 * @note Requires PROC_LOCK.
 */
struct pgrp * proc_session_search_pg(struct session * s, pid_t pg_id);

/**
 * Create a new process group.
 * @param s is a pointer to the session that should contain the new process
 *          group, if s is NULL then a new session is created.
 * @param proc is a pointer to the process linked with the new process group.
 * @note Requires PROC_LOCK.
 */
struct pgrp * proc_pgrp_create(struct session * s, struct proc_info * proc);

/**
 * Insert a process into the process group pgrp.
 * @note Requires PROC_LOCK.
 */
void proc_pgrp_insert(struct pgrp * pgrp, struct proc_info * proc);

/**
 * Remove a process group reference.
 * @note Requires PROC_LOCK.
 */
void proc_pgrp_remove(struct proc_info * proc);

#endif /* PROC_INTERNAL */

/**
 * @}
 */

#endif /* PROC_H */

/**
 * @}
 */

