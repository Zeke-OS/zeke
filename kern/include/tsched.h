/**
 *******************************************************************************
 * @file    tsched.h
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler header file for sched.c.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/** @addtogroup sched
  * @{
  */

#pragma once
#ifndef TSCHED_H
#define TSCHED_H

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif
#include <autoconf.h>
#include <sched.h>
#include <ksignal.h>
#include <vm/vm.h>
#include <hal/core.h>

/*
 * Scheduler flags
 * ===============
 *
 * SCHED_IN_USE_FLAG
 * 1 = Thread is in use and can be rescheduled to execute at some point.
 * 0 = Thread is in zombie state or already removed from the system.
 *
 * SCHED_WAIT_FLAG
 * Thread is waiting for one or more events. This flag should be removed only
 * if wait_count is 0.
 * 0 = Thread is not wating for events.
 * 1 = Thread is waiting for events.
 *
 * SCHED_NO_SIG_FLAG
 * Thread cannot be woken up by a signal if this flag is set.
 *
 * SCHED_INTERNAL_FLAG
 * Thread cannot be killed if this flag is set.
 */
#define SCHED_IN_USE_FLAG   0x00000001u /*!< IN USE FLAG */
#define SCHED_EXEC_FLAG     0x00000002u /*!< EXEC = 1 / SLEEP = 0 */
#define SCHED_WAIT_FLAG     0x00000004u /*!< Waiting for kworker or IO. */
#define SCHED_NO_SIG_FLAG   0x00000008u /*!< Thread cannot be woken up by a
                                         *   signal. */
#define SCHED_ZOMBIE_FLAG   0x00000010u /*!< Zombie waiting for parent. */
#define SCHED_DETACH_FLAG   0x00000020u /*!< Detached thread. If thread exits
                                         *   it will be immediately destroyed. */
#define SCHED_KWORKER_FLAG  0x40000000u /*!< Thread is a kworker. */
#define SCHED_INTERNAL_FLAG 0x80000000u /*!< Immortal internal kernel thread. */

/**
 * Context switch ok flags.
 * When these flags are set for a thread it's ok to make a context switch to it.
 */
#define SCHED_CSW_OK_FLAGS \
    (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG)

/**
 * Flags marking that a thread is a detached zombie and can be killed
 * without parent's intervention.
 */
#define SCHED_DETACHED_ZOMBIE_FLAGS \
    (SCHED_IN_USE_FLAG | SCHED_ZOMBIE_FLAG | SCHED_DETACH_FLAG)

/**
 * Test if context switch to a thread is ok based on flags.
 * @param x is the flags of the thread tested.
 */
#define SCHED_TEST_CSW_OK(x)                                            \
    (((x) & (SCHED_CSW_OK_FLAGS | SCHED_WAIT_FLAG | SCHED_ZOMBIE_FLAG)) \
     == SCHED_CSW_OK_FLAGS)

/**
 * Test if waking up a thread is ok based on flags.
 * This also tests that SCHED_EXEC_FLAG is not set as scheduling may break if
 * same thread is put on exection twice.
 * @param x is the flags of the thread tested.
 */
#define SCHED_TEST_WAKEUP_OK(x)                                             \
    (((x) & (SCHED_IN_USE_FLAG | SCHED_EXEC_FLAG | SCHED_ZOMBIE_FLAG        \
             | SCHED_NO_SIG_FLAG | SCHED_WAIT_FLAG)) == SCHED_IN_USE_FLAG)

/**
 * Test if it is ok to terminate.
 * @param x is the flags of the thread tested.
 */
#define SCHED_TEST_TERMINATE_OK(x) \
    (((x) & (SCHED_IN_USE_FLAG | SCHED_INTERNAL_FLAG)) == SCHED_IN_USE_FLAG)

/**
 * Test if thread is a detached zombie.
 * If thread is a detached zombie it can be killed without parent's
 * intervention.
 * @param x is the flags of the thread tested.
 */
#define SCHED_TEST_DETACHED_ZOMBIE(x) \
    (((x) & SCHED_DETACHED_ZOMBIE_FLAGS) == SCHED_DETACHED_ZOMBIE_FLAGS)

/* Stack frames */
#define SCHED_SFRAME_SYS        0   /*!< Sys int/Scheduling stack frame. */
#define SCHED_SFRAME_SVC        1   /*!< Syscall stack frame. */
#define SCHED_SFRAME_ABO        2   /*!< Stack frame for aborts. */
#define SCHED_SFRAME_ARR_SIZE   3

#if configSCHED_CDS != 0
RB_HEAD(sched_threads, thread_info);
RB_HEAD(sched_ready, thread_info);
RB_HEAD(sched_exec, thread_info);
#endif

/**
 * Thread info struct.
 * Thread Control Block structure.
 */
typedef struct thread_info {
    uint32_t flags;                 /*!< Status flags. */
    sw_stack_frame_t sframe[SCHED_SFRAME_ARR_SIZE];
    struct buf * kstack_region;    /*!< Thread kernel stack region. */
    void * errno_uaddr;             /*!< Address of the thread local errno. */
    intptr_t retval;                /*!< Return value of the thread. */
    atomic_t a_wait_count;          /*!< Wait counter. -1 = permanent */
    int wait_tim;                   /*!< Reference to a timeout timer. */
    int niceval;                    /*!< Thread nice value. */
    int priority;                   /*!< Current dynamic priority. */
    int ts_counter;                 /*!< Time slice counter. */
    pthread_t id;                   /*!< Thread id. */
    pid_t pid_owner;                /*!< Owner process of this thread. */

#if configSCHED_CDS != 0
    struct sched {
        unsigned policy;                            /*!< Scheduling policy. */
        RB_ENTRY(thread_info)   threads_entry;
        RB_ENTRY(thread_info)   ready_entry;
        RB_ENTRY(thread_info)   exec_entry;
        llist_nodedsc_t         fifo_exec_entry;
    } sched;
#endif

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
    struct threadInheritance_t {
        void * parent;              /*!< Parent thread */
        void * first_child;         /*!< Link to the first child thread */
        void * next_child;          /*!< Next child of the common parent */
    } inh;
} threadInfo_t;

/* Scheduler task type */
typedef void (*sched_task_t)();

/* External variables **********************************************************/
extern threadInfo_t * current_thread;

/* Public function prototypes ***************************************************/

#if configSCHED_CDS != 0
int sched_tid_comp(struct thread_info * a, struct thread_info * b);
int sched_nice_comp(struct thread_info * a, struct thread_info * b);
int sched_ts_comp(struct thread_info * a, struct thread_info * b);
RB_PROTOTYPE(sched_threads, thread_info, sched.threads, sched_tid_comp);
RB_PROTOTYPE(sched_ready, thread_info, sched.ready, sched_nice_comp);
RB_PROTOTYPE(sched_exec, thread_info, sched.exec, sched_ts_comp);
#endif

/**
 * Return load averages in integer format scaled to 100.
 * @param[out] loads load averages.
 */
void sched_get_loads(uint32_t loads[3]);

/**
 * Get new unused thread id.
 * Returned thread shall be available with sched_get_thread_info() function and
 * shall not have SCHED_IN_USE_FLAG set.
 * @return Returns a new thread id or -EAGAIN.
 */
pthread_t sched_new_tid(void);

/**
 * Get pointer to a threadInfo structure.
 * @param thread_id id of a thread.
 * @return Pointer to a threadInfo structure of a correspondig thread id
 *         or NULL if thread does not exist.
 */
struct thread_info * sched_get_thread_info(pthread_t thread_id);

/**
 * Set thread into execution with its default priority.
 * @param thread_id is the thread id.
 */
void sched_thread_set_exec(pthread_t thread_id);

#define SCHED_PERMASLEEP 1
/**
 * Put the current thread into sleep.
 * @param permanent sets wait flag to keep thread in permanent sleep.
 */
void sched_sleep_current_thread(int permanent);

/**
 * Yield turn.
 * @param sleep_flag sleep immediately if flag is set.
 */
void sched_current_thread_yield(int sleep_flag);

/**
 * Mark thread as detached so it wont be turned into zombie on exit.
 * @param id is a thread id.
 * @return Return -EINVAL if invalid thread id was given; Otherwise zero.
 */
int sched_thread_detach(pthread_t id);

void sched_schedule(void);

/* Functions that are mainly used by syscalls but can be also caleed by
 * other kernel source modules. */

/**
 * Create a new thread.
 * @param thread_def    Thread definitions.
 * @param priv          If set thread is created as a kernel mode thread aka
 *                      kworker; Otherwise user mode is selected.
 */
pthread_t sched_threadCreate(struct _ds_pthread_create * thread_def, int priv);

/**
 * Removes a thread from scheduling.
 * @param tt_id Thread task table id
 */
void sched_thread_remove(pthread_t id);

/**
 * Set thread priority.
 * @param   thread_id Thread id.
 * @param   priority New priority for thread referenced by thread_id.
 * @return  0; -EINVAL.
 */
int sched_thread_set_priority(pthread_t thread_id, int priority);

int sched_thread_get_priority(pthread_t thread_id);

#endif /* TSCHED_H */

/**
  * @}
  */
