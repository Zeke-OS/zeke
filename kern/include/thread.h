/**
 *******************************************************************************
 * @file    thread.h
 * @author  Olli Vanhoja
 * @brief   Public external thread management and scheduling functions.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#pragma once
#ifndef THREAD_H
#define THREAD_H

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <pthread.h>
#include <sched.h>
#include <machine/atomic.h>
#include <hal/core.h>
#include <hal/mmu.h>
#include <ksignal.h>

/* Stack frames */
#define SCHED_SFRAME_SYS        0   /*!< Sys int/Scheduling stack frame. */
#define SCHED_SFRAME_SVC        1   /*!< Syscall stack frame. */
#define SCHED_SFRAME_ABO        2   /*!< Stack frame for aborts. */
#define SCHED_SFRAME_ARR_SIZE   3

/**
 * Thread state.
 */
enum thread_state {
    THREAD_STATE_INIT = 0,          /*!< Thread initial state. */
    THREAD_STATE_READY,             /*!< Thread ready for exec.
                                     *   (in generic readyq) */
    THREAD_STATE_EXEC,              /*!< Thread in execution.
                                     *   (in scheduler) */
    THREAD_STATE_BLOCKED,           /*!< Thread waiting or blocked. */
    THREAD_STATE_DEAD,              /*!< Thread is dead. */
};

/*
 * Scheduler flags
 * ===============
 *
 * SCHED_IN_USE_FLAG
 * 1 = Thread is in use and can be rescheduled to execute at some point.
 * 0 = Thread is beign removed from the system.
 *
 * SCHED_INTERNAL_FLAG
 * Thread cannot be killed if this flag is set.
 */
#define SCHED_IN_USE_FLAG   0x00000001u /*!< Thread struct is in use and valid.
                                         */
#define SCHED_DETACH_FLAG   0x00000002u /*!< Detached thread. If thread exits
                                         *   it will be immediately destroyed
                                         *   without storing the return value.
                                         */
#define SCHED_INSYS_FLAG    0x01000000u /*!< Thread in system call.
                                             In system call if set;
                                         *   Otherwise usr. This can be used for
                                         *   counting process times or as
                                         *   a state information.
                                         */
#define SCHED_KWORKER_FLAG  0x10000000u /*!< Thread is a kworker. */
#define SCHED_INTERNAL_FLAG 0x20000000u /*!< Immortal internal kernel thread. */

/**
 * Thread state info struct.
 * Thread Control Block structure.
 */
struct thread_info {
    pthread_t id;                   /*!< Thread id. */
    uint32_t flags;                 /*!< Status flags. */
    pid_t pid_owner;                /*!< Owner process of this thread. */

    /**
     * Scheduler data,
     * Data that shouldn't be cloned to any other thread.
     */
    struct sched_thread_data {
        enum thread_state state;
        unsigned policy_flags;
        int ts_counter;
        mtx_t tdlock;                   /*!< Lock for data in this substruct. */
        RB_ENTRY(thread_info) ttentry_; /*!< Thread table entry. */
        STAILQ_ENTRY(thread_info) readyq_entry_;
        TAILQ_ENTRY(thread_info) rrrunq_entry_;
    } sched;

    sw_stack_frame_t sframe[SCHED_SFRAME_ARR_SIZE];
    struct buf * kstack_region;     /*!< Thread kernel stack region. */
    mmu_pagetable_t * curr_mpt;     /*!< Current master pt (proc or kern) */
    void * errno_uaddr;             /*!< Address of the thread local errno. */
    intptr_t retval;                /*!< Return value of the thread. */

    atomic_t a_wait_count;          /*!< Wait counter.
                                     *   value < 0 is permanent sleep. */
    int wait_tim;                   /*!< Reference to a timeout timer. */
    struct sched_param param;       /*!< Scheduling parameters set by user. */

    /* Signals */
    struct signals sigs;            /*!< Signals. */
    int sigwait_retval;             /*!< Return value for sigwait() */

    /**
     * Thread inheritance; Parent and child thread pointers.
     *
     *  inh : Parent and child thread relations
     *  ---------------------------------------
     * + first_child is a parent thread attribute containing address to a first
     *   child of the parent thread
     * + parent is a child thread attribute containing address to a parent
     *   thread of the child thread
     * + next_child is a child thread attribute containing address of a next
     *   child node of the common parent thread
     */
    struct thread_inheritance {
        /** Parent thread */
        struct thread_info * parent;
        /** Link to the first child thread. */
        struct thread_info * first_child;
        /** Next child of the common parent. */
        struct thread_info * next_child;
    } inh;
};

/**
 * Thread yield strategy.
 * Immediate yield will not return to the caller until other threads have been
 * scheduled in if any; Lazy yield will yield turn at some point but it may
 * also return to the caller at first.
 */
enum thread_eyield_strategy {
    THREAD_YIELD_IMMEDIATE,
    THREAD_YIELD_LAZY
};

/**
 * Type for thread constructor and destructor functions.
 */
typedef void (*thread_cdtor_t)(struct thread_info * td);

/* External variables *********************************************************/
extern struct thread_info * current_thread;

/**
 * Compare two thread_info structs.
 * @param a is the left node.
 * @param b is the right node.
 * @return  If the first argument is smaller than the second, the function
 *          returns a value smaller than zero;
 *          If they are equal, the  function returns zero;
 *          Otherwise, value greater than zero is returned.
 */
int thread_id_compare(struct thread_info * a, struct thread_info * b);

/**
 * @addtogroup thread_flags
 * Thread flags manipulation functions.
 * @{
 */

/**
 * Set flags indicated by flags_mask.
 */
int thread_flags_set(struct thread_info * thread, uint32_t flags_mask);

/**
 * Clear flags indicated by flags_mask.
 */
int thread_flags_clear(struct thread_info * thread, uint32_t flags_mask);

/**
 * Get current thread flags.
 */
uint32_t thread_flags_get(struct thread_info * thread);

int thread_flags_is_set(struct thread_info * thread, uint32_t flags_mask);

int thread_flags_not_set(struct thread_info * thread, uint32_t flags_mask);

/**
 * Get thread state.
 * @param thread is a pointer to the thread.
 * @return Returns the current thread state.
 */
enum thread_state thread_state_get(struct thread_info * thread);

/**
 * Set thread state.
 * @param thread is a pointer to the thread.
 * @param state is the new thread state.
 * @return Returns the old thread state.
 */
enum thread_state thread_state_set(struct thread_info * thread,
                                   enum thread_state state);

/**
 * @}
 */

/**
 * Create a new thread.
 * @param thread_def    Thread definitions.
 * @param priv          If set thread is created as a kernel mode thread aka
 *                      kworker; Otherwise user mode is selected.
 */
pthread_t thread_create(struct _sched_pthread_create_args * thread_def,
                        int priv);

/**
 * Get pointer to a thread_info structure.
 * @param thread_id id of a thread.
 * @return Pointer to a thread_info structure of a correspondig thread id
 *         or NULL if thread does not exist.
 */
struct thread_info * thread_lookup(pthread_t thread_id);

/**
 * Fork current thread.
 * @note Cloned thread is set to sleep state and caller of this function should
 * set it to exec state. Caller is also expected to handle user stack issues as
 * as well. The new thread is exact clone of the current thread but with a new
 * kernel stack.
 * @return  0 clone succeed and this is the new thread executing;
 *          < 0 error;
 *          > 0 clone succeed and return value is the id of the new thread.
 */
pthread_t thread_fork(void);

/**
 * Mark thread ready.
 */
int thread_ready(pthread_t thread_id);

/**
 * Remove first thread that's ready.
 */
struct thread_info * thread_remove_ready(void);

/**
 * Wait for event.
 * Put current_thread on sleep until thread_release().
 * Can be called multiple times, eg. from interrupt handler.
 */
void thread_wait(void);

/**
 * Release thread waiting state.
 */
void thread_release(struct thread_info * thread);

/**
 * Sleep current thread for millisec.
 * @param millisec is the sleep time.
 */
void thread_sleep(long millisec);

/**
 * Yield turn.
 */
void thread_yield(enum thread_eyield_strategy strategy);

/**
 * Terminate current thread.
 * This makes current_thread a zombie that should be either killed by the
 * parent thread or will be killed at least when the parent is killed.
 * @param retval is a return value from the thread.
 */
void thread_die(intptr_t retval);

/**
 * Get thread id of the current thread.
 */
pthread_t get_current_tid(void);

/**
 * Get a stack frame of the current thread.
 * @param ind is the stack frame index.
 * @return  Returns an address to the stack frame of the current thread;
 *          Or NULL if current_thread is not set.
 */
void * thread_get_curr_stackframe(size_t ind);

/**
 * Set thread scheduling policy.
 */
int thread_set_policy(pthread_t thread_id, unsigned policy);

/**
 * Get thread scheduling policy.
 */
unsigned thread_get_policy(pthread_t thread_id);

/**
 * Set thread priority.
 * @param   thread_id Thread id.
 * @param   priority New priority for thread referenced by thread_id.
 * @return  0; -EINVAL.
 */
int thread_set_priority(pthread_t thread_id, int priority);

/**
 * Get thread priority.
 * @param thread_id is the thread id.
 * @return Returns the thread priory value or NICE_ERR if thread doesn't exist.
 */
int thread_get_priority(pthread_t thread_id);

/**
 * Terminate a thread and its childs.
 * @param thread_id     thread ID obtained by \ref sched_thread_create or
 *                      \ref sched_thread_getId.
 * @return 0 if succeed; Otherwise -EPERM.
 */
int thread_terminate(pthread_t thread_id);

/**
 * Permanently remove a thread from scheduling and free its resources.
 * @param thread_id Thread task table id
 */
void thread_remove(pthread_t thread_id);

#endif /* THREAD_H */
