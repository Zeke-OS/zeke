/**
 *******************************************************************************
 * @file    thread.h
 * @author  Olli Vanhoja
 * @brief   Generic thread management and scheduling functions.
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

#pragma once
#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <sys/types.h>
#include <tsched.h>

typedef void (*thread_cdtor_t)(struct thread_info * td);

/**
 * Scheduler handler.
 * This is mainly called when timer ticks.
 */
void sched_handler(void);

/**
 * Kernel idle thread.
 */
void * idle_thread(void * arg);

/**
 * Create a new thread.
 * @param thread_def    Thread definitions.
 * @param priv          If set thread is created as a kernel mode thread aka
 *                      kworker; Otherwise user mode is selected.
 */
pthread_t thread_create(struct _ds_pthread_create * thread_def, int priv);

/**
  * Set thread initial configuration
  * @note This function should not be called for already initialized threads.
  * @param tp           is a pointer to the thread struct.
  * @param thread_id    Thread id
  * @param thread_def   Thread definitions
  * @param parent       Parent thread id, NULL = doesn't have a parent
  * @param priv         If set thread is initialized as a kernel mode thread
  *                     (kworker).
  * @todo what if parent is stopped before this function is called?
  */
void thread_init(struct thread_info * tp, pthread_t thread_id,
                 struct _ds_pthread_create * thread_def, threadInfo_t * parent,
                 int priv);

/**
 * Set In system call state for current thread.
 * @param s
 */
void thread_set_current_insys(int s);

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
 * Wait for event.
 * Put current_thread on sleep until thread_release().
 * Can be called multiple times, eg. from interrupt handler.
 */
void thread_wait(void);

/**
 * Release thread waiting state.
 */
void thread_release(threadInfo_t * thread);

void thread_sleep(long millisec);

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
 * Terminate current thread.
 * This makes current_thread a zombie that should be either killed by the
 * parent thread or will be killed at least when the parent is killed.
 * @param retval is a return value from the thread.
 */
void thread_die(intptr_t retval);

/**
 * Terminate a thread and its childs.
 * @param thread_id     thread ID obtained by \ref sched_thread_create or
 *                      \ref sched_thread_getId.
 * @return 0 if succeed; Otherwise -EPERM.
 */
int thread_terminate(pthread_t thread_id);

#endif /* THREAD_H */
