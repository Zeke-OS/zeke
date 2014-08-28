/**
 *******************************************************************************
 * @file    pthread.h
 * @author  Olli Vanhoja
 * @brief   Threads.
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
#ifndef TYPES_PTHREAD_H
#define TYPES_PTHREAD_H

#include <stddef.h>
#include <stdint.h>

typedef int pthread_t; /*!< Thread ID. */

/* TODO Missing types:
 * - pthread_barrier_t
 * - pthread_barrierattr_t
 * - pthread_cond_t
 * - pthread_condattr_t
 * - pthread_key_t
 * - pthread_mutex_t
 * - pthread_mutexattr_t
 * - pthread_once_t
 * - pthread_rwlock_t
 * - pthread_rwlockattr_t
 * - pthread_spinlock_t
 */

/**
 * Entry point of a thread.
 */
typedef void * (*start_routine)(void *);

/**
 * Thread Definition structure contains startup information of a thread.
 */
typedef const struct pthread_attr {
    int             tpriority;  /*!< initial thread priority */
    void *          stackAddr;  /*!< Stack address */
    size_t          stackSize;  /*!< Size of stack reserved for the thread. */
} pthread_attr_t;

enum os_mutex_strategy {
    os_mutex_str_reschedule,
    os_mutex_str_sleep
};

/**
 * Mutex Definition structure contains setup information for a mutex.
 */
typedef struct {
    enum os_mutex_strategy strategy;
} pthread_mutexattr_t;

typedef struct {
    volatile pthread_t thread_id; /*!< ID of the thread holding the lock */
    volatile int lock; /*!< Lock variable */
    enum os_mutex_strategy strategy; /*!< Locking strategy */
} pthread_mutex_t;

#endif /* TYPES_PTHREAD_H */
