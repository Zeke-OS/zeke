/**
 *******************************************************************************
 * @file    thread.h
 * @author  Olli Vanhoja
 * @brief   Public external thread management and scheduling functions.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 */
enum {
    /**
     * Thread struct is in use and valid.
     * 1 = Thread is in use and can be rescheduled to execute at some point.
     * 0 = Thread is beign removed from the system.
     */
    SCHED_IN_USE_FLAG   = 1 << 0,
    /**
     * Detached thread.
     * If thread exits it will be immediately destroyed without storing
     * the return value or any statistics.
     */
    SCHED_DETACH_FLAG   = 1 << 1,
    /**
    * Thread is in a system call.
    */
    SCHED_INSYS_FLAG    = 1 << 2,
    /**
     * Thread is in abort mode.
     */
    SCHED_INABO_FLAG    = 1 << 3,
    /**
    * Thread is a kworker.
    */
    SCHED_KWORKER_FLAG  = 1 << 4,
    /**
     * Immortal internal kernel thread.
     * A thread cannot be killed if this flag is set.
     */
    SCHED_INTERNAL_FLAG = 1 << 5,
    /**
     * Yield execution turn to the next thread.
     */
    SCHED_YIELD_FLAG = 1 << 6,
};

/**
 * No timer assigned.
 */
#define TMNOVAL (-1)

/**
 * Thread state info struct.
 * Thread Control Block structure.
 */
struct thread_info {
    pthread_t id;                   /*!< Thread id. */
    uint32_t flags;                 /*!< Status flags. */
    pid_t pid_owner;                /*!< Owner process of this thread. */
    char name[_ZEKE_THREAD_NAME_SIZE]; /*!< Thread name. */

    /**
     * Scheduler data.
     * Data that shouldn't be cloned to any other thread.
     */
    struct sched_thread_data {
        enum thread_state state;    /*!< Thread execution state. */
        unsigned policy_flags;      /*!< Scheduling policy specific flags */
        int ts_counter;             /*!< Thread time slice counter;
                                     *   Set to -1 if not used. */
        mtx_t tdlock;               /*!< Lock for data in this substruct. */
        RB_ENTRY(thread_info) ttentry_; /*!< Thread table entry. */
        STAILQ_ENTRY(thread_info) readyq_entry_;

        /* Scheduling policy specific data. */
        union {
            /* FIFO policy */
            struct thread_sched_fifo {
                int prio;           /*!< EXEC time priority. */
                RB_ENTRY(thread_info) runq_entry_;
            } fifo;
            /* RR policy */
            struct thread_sched_rr {
                TAILQ_ENTRY(thread_info) runq_entry_;
            } rr;
        };
    } sched;
    struct sched_param param;       /*!< Scheduling parameters set by user. */

    /* Timers */
    int wait_tim;                   /*!< Reference to a timeout timer. */
    int lock_tim;                   /*!< Timer used by klocks. */

    thread_stack_frames_t sframe;
    struct tls_regs tls_regs;       /*!< Thread local registers. */
    struct buf * kstack_region;     /*!< Thread kernel stack region. */
    mmu_pagetable_t * curr_mpt;     /*!< Current master pt (proc or kern) */
    __user struct _sched_tls_desc * tls_uaddr; /*!< Thread local storage. */
    intptr_t retval;                /*!< Return value of the thread. */
    struct ksiginfo * exit_ksiginfo; /*!< The signal that killed the thread.
                                          (NULL if wasn't killed) */

    /* Signals */
    struct signals sigs;            /*!< Signals. */
    struct ksiginfo * sigwait_retval; /*!< Return value for sigwait(). */

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
    THREAD_YIELD_IMMEDIATE, /*!< Immediate yield. */
    THREAD_YIELD_LAZY       /*!< Yield when the next suitable tick occurs. */
};

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

/**
 * Test that selected thread state flags are set.
 */
int thread_flags_is_set(struct thread_info * thread, uint32_t flags_mask);

/**
 * Test that none of the selected thread state flags are set.
 */
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
 * Test if a policy flag is set.
 */
static inline int thread_test_polflag(struct thread_info * thread,
                                      unsigned flag)
{
    return ((thread->sched.policy_flags & flag) == flag);
}

/**
 * Thread creation mode.
 */
enum thread_mode {
    THREAD_MODE_USER, /*!< Regular user thread. */
    THREAD_MODE_PRIV, /*!< Privileged kernel thread. */
};

/**
 * Create a new thread.
 * @param thread_def    Thread definitions.
 * @param priv          If set thread is created as a kernel mode thread aka
 *                      kworker; Otherwise user mode is selected.
 * @return  >= 0 Thread id of the newly created thread;
 *           < 0 Otherwise a negative errno code is returned.
 */
pthread_t thread_create(struct _sched_pthread_create_args * thread_def,
                        enum thread_mode thread_mode);

/**
 * Create a simple detached kernel thread.
 * @param stack_size    selects the allocated stack size; If the value is zero
 *                      then the default size is used.
 */
pthread_t kthread_create(char * name, struct sched_param * param,
                         size_t stack_size,
                         void * (*kthread_start)(void *), void * arg);

/**
 * Get pointer to a thread_info structure.
 * @note struct thread_info pointer is guaranteed to be valid until next sleep,
 * therefore a thread taking a pointer by using this function should not sleep.
 * @param thread_id id of a thread.
 * @return Pointer to a thread_info structure of a correspondig thread id
 *         or NULL if thread does not exist.
 */
struct thread_info * thread_lookup(pthread_t thread_id)
    __attribute__((warn_unused_result));

/**
 * Fork the current thread.
 * @note Cloned thread is set to sleep state and caller of this function should
 * set it to exec state. Caller is also expected to handle user stack issues as
 * as well. The new thread is exact clone of the current thread but with a new
 * kernel stack.
 * @param new_pid is the new process id, container of the thread.
 * @return If the current thread was forked succesfully the function returns
 *         a pointer to the new thread_info struct;
 *         Otherwise returns a NULL pointer.
 */
struct thread_info * thread_fork(pid_t new_pid)
    __attribute__((warn_unused_result));

/**
 * Mark a thread ready for scheduling.
 * Inserts the thread to a per CPU readyq.
 */
int thread_ready(pthread_t thread_id);

/**
 * Remove first thread that's ready.
 */
struct thread_info * thread_remove_ready(void);

/**
 * Wait for an event.
 * Put current_thread on sleep until thread_release().
 * Can be called multiple times, eg. from interrupt handler.
 */
void thread_wait(void);

/**
 * Release a waiting thread.
 */
void thread_release(pthread_t thread_id);

/**
 * Sleep the current thread for millisec.
 * @param millisec is the sleep time.
 */
void thread_sleep(long millisec);

/**
 * Wake-up the current thread if it's sleeping after millisec.
 * The timer created with this function shall be relased by calling
 * thread_alarm_rele().
 * @returns Returns an id that shall be used to release the timer.
 */
int thread_alarm(long millisec);

/**
 * Release the alarm timer created with thread_alarm().
 */
void thread_alarm_rele(int timer_id);

/**
 * Yield turn.
 */
void thread_yield(enum thread_eyield_strategy strategy);

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
 * @return Returns the thread priory value;
 *         Or NICE_ERR if the thread doesn't exist.
 */
int thread_get_priority(pthread_t thread_id);

/**
 * Get thread effective scheduling priority.
 * @param thread_id is a pointer to the thread descriptor.
 * @return Returns the thread priory value;
 *         Or NICE_ERR if the athread doesn't exist.
 */
int thread_p_get_scheduling_priority(struct thread_info * thread);

/**
 * Get thread effective scheduling priority.
 * @param thread_id is the thread id.
 * @return Returns the thread priory value;
 *         Or NICE_ERR if the athread doesn't exist.
 */
static inline int thread_get_scheduling_priority(pthread_t thread_id)
{
    return thread_p_get_scheduling_priority(thread_lookup(thread_id));
}

/**
 * Terminate current thread.
 * This makes current_thread a zombie that should be either killed by the
 * parent thread or will be killed at least when the parent is killed.
 * @param retval is a return value from the thread.
 */
void thread_die(intptr_t retval);

/**
 * Wait for thread to die or to be terminated.
 * @param thread_id is the thread id.
 * @param[out] retval returns the thread return value.
 * @return 0; Negative errno.
 */
int thread_join(pthread_t thread_id, intptr_t * retval);

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
