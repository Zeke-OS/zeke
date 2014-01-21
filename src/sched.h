/**
 *******************************************************************************
 * @file    sched.h
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler header file for sched.c.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup Scheduler
  * @{
  */

#pragma once
#ifndef SCHED_H
#define SCHED_H

#include <autoconf.h>
#include <syscalldef.h>
#include <kernel.h>

#define SCHED_IN_USE_FLAG   0x00000001u /*!< IN USE FLAG */
#define SCHED_EXEC_FLAG     0x00000002u /*!< EXEC = 1 / SLEEP = 0 */
#define SCHED_NO_SIG_FLAG   0x00000004u /*!< Thread cannot be woken up by a signal. */
#define SCHED_INTERNAL_FLAG 0x00000008u /*!< Immortal internal kernel thread. */

/* When these flags are both set for a it's ok to make a context switch to it. */
#define SCHED_CSW_OK_FLAGS  (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG)

#define SCHED_DEV_WAIT_BIT 0x40000000 /*!< Dev wait signal bit. */

/** Thread info struct
 *
 * Thread Control Block structure.
 */
typedef struct {
    void * sp;                  /*!< Stack pointer */
    uint32_t flags;             /*!< Status flags */
    int32_t signals;            /*!< Signal flags
                                 * @note signal bit 30 is reserved for dev. */
    //int32_t sig_wait_mask;      /*!< Signal wait mask */
    int errno;                  /*!< Thread local errno */
    int wait_tim;               /*!< Reference to a timeout timer */
    osEvent event;              /*!< Event struct */
    osPriority def_priority;    /*!< Thread priority */
    osPriority priority;        /*!< Thread dynamic priority */
    int ts_counter;             /*!< Time slice counter */
    pthread_t id;               /*!< Thread id */
    pid_t pid_owner;            /*!< Owner process of this thread */

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
    volatile struct threadInheritance_t {
        void * parent;              /*!< Parent thread */
        void * first_child;         /*!< Link to the first child thread */
        void * next_child;          /*!< Next child of the common parent */
    } inh;
} threadInfo_t;

/* External variables **********************************************************/
extern volatile uint32_t sched_enabled;
extern volatile threadInfo_t * current_thread;

/* Public function prototypes ***************************************************/
/**
 * Start the scheduler
 */
void sched_start(void);

/**
 * Scheduler handler
 *
 * Scheduler handler is mainly called due to sysTick and PendSV
 * interrupt and always by an interrupt handler.
 * @param tsp Current thread stack pointer.
 * @return    Next thread stack pointer.
 */
void * sched_handler(void * tsp);

/**
 * Get pointer to a threadInfo structure.
 * @param thread_id id of a thread.
 * @return Pointer to a threadInfo structure of a correspondig thread id
 *         or NULL if thread does not exist.
 */
threadInfo_t * sched_get_pThreadInfo(int thread_id);

/**
 * Set thread into execution mode/ready to run mode.
 * @oaram thread_id Thread id.
 */
void sched_thread_set_exec(int thread_id);

/**
 * Put the current thread into sleep.
 */
void sched_thread_sleep_current(void);

/**
 * Return load averages in integer format scaled to 100.
 * @param[out] loads load averages.
 */
void sched_get_loads(uint32_t loads[3]);

/**
  * @}
  */

/** @addtogroup Kernel
  * @{
  */

/** @addtogroup External_routines
  * @{
  */

/* Functions that are mainly used by syscalls but can be also caleed by
 * other kernel source modules. */

/**
 * Create a new thread
 *
 */
pthread_t sched_threadCreate(ds_pthread_create_t * thread_def);

/**
 * Get id of the currently running thread
 * @return Thread id of the current thread
 */
pthread_t sched_thread_getId(void);

/**
 * Terminate a thread and its childs.
 * @param thread_id   thread ID obtained by \ref sched_threadCreate or \ref sched_thread_getId.
 * @return status code that indicates the execution status of the function.
 */
osStatus sched_thread_terminate(pthread_t thread_id);

/**
 * Set thread priority
 * @param   thread_id Thread id
 * @param   priority New priority for thread referenced by thread_id
 * @return  osOK if thread exists
 */
osStatus sched_thread_setPriority(pthread_t thread_id, osPriority priority);

osPriority sched_thread_getPriority(pthread_t thread_id);

osStatus sched_threadDelay(uint32_t millisec);

/**
 * Thread wait syscall handler
 * @param millisec Event wait time in ms or osWaitForever.
 * @note This function returns a pointer to a thread event struct and its
 * contents is allowed to change before returning back to the caller thread.
 */
osStatus sched_threadWait(uint32_t millisec);

/* Syscall handlers */
uint32_t sched_syscall(uint32_t type, void * p);
uint32_t sched_syscall_thread(uint32_t type, void * p);
uint32_t sched_syscall_signal(uint32_t type, void * p);

#endif /* SCHED_H */

/**
  * @}
  */

/**
  * @}
  */
