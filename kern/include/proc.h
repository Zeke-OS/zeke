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
#include <ksignal.h>
#include <vm/vm.h>

/**
 * Process state.
 */
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

/**
 * Maximum size of the process name.
 */
#define PROC_NAME_SIZE 16

struct mmu_abo_param;
struct thread_info;

/**
 * Session descriptor.
 */
struct session {
    pid_t s_leader;             /*!< Session leader. */
    int s_pgrp_count;
    int s_ctty_fd;              /*!< fd number of the controlling terminal. */
    char s_login[MAXLOGNAME];   /*!< Setlogin() name. */
    TAILQ_HEAD(pgrp_list, pgrp) s_pgrp_list_head; /*!< List of pgroups in this
                                                   *   session. */
    TAILQ_ENTRY(session) s_session_list_entry_; /*!< For the list of all
                                                 *   sessions. */
};

/**
 * Process group descriptor.
 */
struct pgrp {
    pid_t pg_id;                /*!< Pgrp id. */
    int pg_proc_count;
    struct session * pg_session; /*!< Pointer to the session. */
    TAILQ_HEAD(proc_list, proc_info) pg_proc_list_head;
    TAILQ_ENTRY(pgrp) pg_pgrp_entry_;
};

/**
 * Process Control Block.
 */
struct proc_info {
    pid_t pid;
    char name[PROC_NAME_SIZE];  /*!< Process name. */
    enum proc_state state;      /*!< Process state. */
    int nice;                   /*!< Niceness. */
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
        struct proc_info * parent; /*!< A pointer to the parent process. */
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

/**
 * Traverse the list of child processes.
 */
#define PROC_INH_FOREACH(_var, _proc) \
    SLIST_FOREACH((_var), PROC_INH_HEAD(_proc), inh.child_list_entry)

/**
 * Traverse the list of child processes starting from _var.
 */
#define PROC_INH_FOREACH_FROM(_var, _proc) \
    SLIST_FOREACH_FROM((_var), PROC_INH_HEAD(_proc), inh.child_list_entry)

/**
 * Traverse the list of child processes.
 */
#define PROC_INH_FOREACH_SAFE(_var, _tmp_var, _proc) \
    SLIST_FOREACH_SAFE((_var), PROC_INH_HEAD(_proc), inh.child_list_entry, \
                       (_tmp_var))

/**
 * Traverse the list of child processes starting from _var.
 */
#define PROC_INH_FOREACH_FROM_SAFE(_var, _tmp_var, _proc) \
    SLIST_FOREACH_SAFE((_var), PROC_INH_HEAD(_proc), inh.child_list_entry, \
                       (_tmp_var))

/**
 * Iinitialize the list of child processes.
 */
#define PROC_INH_INIT(_proc) SLIST_INIT(PROC_INH_HEAD(_proc))

/**
 * Insert a child process after _elm1.
 */
#define PROC_INH_INSERT_AFTER(_elm1, _elm2) \
    SLIST_INSERT_AFTER((_elm1), _elm2, inh.child_list_entry)

/**
 * Insert a child process to the head of the list.
 */
#define PROC_INH_INSERT_HEAD(_proc, _elm) \
    SLIST_INSERT_HEAD(PROC_INH_HEAD(_proc), (_elm), inh.child_list_entry)

/**
 * Get the next child process.
 */
#define PROC_INH_NEXT(_elm) \
    SLIST_NEXT((_elm), inh.child_list_entry)

/**
 * Remove the child process after _elm.
 */
#define PROC_INH_REMOVE_AFTER(_elm) \
    SLIST_REMOVE_AFTER((_elm), inh.child_list_entry)

/**
 * Remove the first child process.
 */
#define PROC_INH_REMOVE_HEAD(_proc) \
    SLIST_REMOVE_HEAD(PROC_INH_HEAD(_proc), inh.child_list_entry)

/**
 * Remove a child process.
 */
#define PROC_INH_REMOVE(_proc, _elm) \
    SLIST_REMOVE(PROC_INH_HEAD(_proc), (_elm), proc_info, inh.child_list_entry)

/**
 * Swap the children of _proc1 with _proc2.
 */
#define PROC_INH_SWAP(_proc1, _proc2) \
    SLIST_SWAP(PROC_INH_HEAD(_proc1), PROC_INH_HEAD(_proc2), \
               inh.child_list_entry)

/**
 * Lock type used for a inheritance list synchronization.
 * @{
 */

/**
 * Type of the proc_inh lock.
 */
#define PROC_INH_LOCK_TYPE (MTX_TYPE_SPIN)

/**
 * Options specified for inh_lock.
 */
#define PROC_INH_LOCK_OPT  (0)

/**
 * @}
 */

extern int nprocs;                  /*!< Current # of procs. */
extern struct proc_info * curproc;  /*!< PCB of the current process. */

/**
 * Giant lock for proc related things.
 * Protects proc array, data structures and variables in proc.
 * @{
 */

/**
 * proclock.
 * Protects proc array, data structures and variables in proc.
 * This should be only touched by using macros defined in proc.h file.
 */
extern mtx_t proclock;

/**
 * Take a lock to proclock.
 */
#define PROC_LOCK()         mtx_lock(&proclock)

/**
 * Release the lock to proclock.
 */
#define PROC_UNLOCK()       mtx_unlock(&proclock)

/**
 * Test wheter proclock is locked.
 */
#define PROC_TESTLOCK()     mtx_test(&proclock)

/**
 * @}
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
int proc_exists(pid_t pid);

/**
 * Test if process exists.
 */
int proc_exists_locked(pid_t pid);

/**
 * Get a reference to a proc_info struct.
 */
struct proc_info * proc_ref(pid_t pid) __attribute__((warn_unused_result));

/**
 * Get a reference to a proc_info struct.
 */
struct proc_info * proc_ref_locked(pid_t pid)
    __attribute__((warn_unused_result));

/**
 * Uref a proc.
 * @note Handles a NULL pointer.
 */
void proc_unref(struct proc_info * proc);

/**
 * Process state enum to string name of the state.
 * @param state is the process state enum.
 * @return A C-string describing the process state is returned.
 */
const char * proc_state2str(enum proc_state state);

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

TAILQ_HEAD(proc_session_list, session); /*!< Type of the session list. */
struct proc_session_list proc_session_list_head; /*!< List of sessions. */
extern int nr_sessions; /*!< Number of sessions. */

/**
 * Test if a process is the session leader.
 * @param p is a pointer to the process.
 * @return  A boolean true is returned if the process pointed by p is the
 *          session leader; Otherwise a boolean false is returned.
 */
static inline int proc_is_session_leader(struct proc_info * p)
{
    return (p->pid == p->pgrp->pg_session->s_leader);
}

#ifdef PROC_INTERNAL

/**
 * Set login name of the session.
 * @param s is a pointer to the session descriptor.
 * @param[in] s_login is a pointer to the string to be used as a login name.
 *                    The string is copied to the session struct.
 */
void proc_session_setlogin(struct session * s, char s_login[MAXLOGNAME]);

/**
 * Search for a process group in a session.
 * @note Requires PROC_LOCK.
 * @param s is a pointer to a session descriptor.
 * @param pg_id is the process group number to be searched for.
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
 * @param proc is a pointer to the process.
 */
void proc_pgrp_insert(struct pgrp * pgrp, struct proc_info * proc);

/**
 * Remove a process group reference.
 * @note Requires PROC_LOCK.
 * @param proc is a pointer to the process.
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

