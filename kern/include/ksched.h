/**
 *******************************************************************************
 * @file    ksched.h
 * @author  Olli Vanhoja
 * @brief   Kernel thread scheduler header file.
 * @section LICENSE
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

/**
 * @addtogroup sched
 * @{
 */

#pragma once
#ifndef KSCHED_H
#define KSCHED_H

#include <stdint.h>
#include <sched.h>

struct thread_info;

#define KSCHED_CPU_COUNT    1

/**
 * Struct describing a generic thread scheduler.
 * Calls to insert and run for a single thread are always made
 * from the same CPU, interrupts disabled.
 */
struct scheduler {
    char name[10];  /*!< Scheduler name. */

    /**
     * Insert thread for scheduling with this policy.
     * @param sobj is a pointer to the scheduling object.
     * @param thread is a pointer to the thread to be inserted.
     * @return  Zero if succeeded; Otherwise a negative errno code is returned.
     */
    int (*insert)(struct scheduler * sobj, struct thread_info * thread);
    /**
     * Run the scheduler.
     * @param sobj is a pointer to the scheduling object.
     * @return  Returns the next thread to be executed;
     *          Or a NULL pointer if the next thread can't be selected.
     * @return  A pointer to the next thread; If there is no schedudable threads
     *          a NULL pointer is returned.
     */
    struct thread_info * (*run)(struct scheduler * sobj);
    /**
     * Get number of active threads in scheduling by this scheduler object.
     * @param sobj is a pointer to the scheduling object.
     * @return  Number of threads scheduled in the context of sobj.
     */
    unsigned (*get_nr_active_threads)(struct scheduler * sobj);
};

/**
 * The type of scheduler constructor creating a new thread scheduler object.
 * @return  Returns a pointer to a new thread scheduler; Otherwise -ENOMEM.
 */
typedef struct scheduler * sched_constructor(void);

/**
 * Scheduler task type.
 */
typedef void sched_task_t(void);

/**
 * Type for thread constructor and destructor functions.
 * @param td    is a pointer to the new thread control block.
 */
typedef void thread_cdtor_t(struct thread_info * td);

/**
 * Type for thread fork handler functions.
 * @param td    is a pointer to the new thread control block.
 * @param old   is a pointer to the old thread control block.
 */
typedef void thread_fork_handler_t(struct thread_info * td,
                                   struct thread_info * old);

#define SCHED_PRE_SCHED_TASK(fun)       \
    DATA_SET(pre_sched_tasks, fun)

#define SCHED_POST_SCHED_TASK(fun)      \
    DATA_SET(post_sched_tasks, fun)

/**
 * Declare a thread constructor function.
 */
#define SCHED_THREAD_CTOR(fun)          \
    DATA_SET(thread_ctors, fun)

/**
 * Declare a thread destructor function.
 */
#define SCHED_THREAD_DTOR(fun)          \
    DATA_SET(thread_dtors, fun)

/**
 * Declare a thread fork handler function.
 */
#define SCHED_THREAD_FORK_HANDLER(fun)  \
    DATA_SET(thread_fork_handlers, fun)

#ifdef _SYS_SYSCTL_H_
SYSCTL_DECL(_kern_sched);
#endif

/**
 * Get index of the current CPU core.
 */
int get_cpu_index(void);

/**
 * Return load averages in integer format scaled to 100.
 * @param[out] loads load averages.
 */
void sched_get_loads(uint32_t loads[3]);

void sched_handler(void);

#endif /* KSCHED_H */

/**
 * @}
 */
