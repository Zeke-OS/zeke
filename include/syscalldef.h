/**
 *******************************************************************************
 * @file    syscalldef.h
 * @author  Olli Vanhoja
 * @brief   Types and definitions for syscalls.
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

#pragma once
#ifndef SYSCALLDEF_H
#define SYSCALLDEF_H

#include <kernel.h>

/** Argument struct for SYSCALL_SCHED_THREAD_CREATE */
typedef struct {
    pthread_t * thread;     /*!< Returned thread id. */
    start_routine start;    /*!< Thread start routine. */
    pthread_attr_t * def;   /*!< Thread def attributes. */
    void * argument;        /*!< Thread parameter(s) pointer. */
} ds_pthread_create_t;

/** Argument struct for SYSCALL_SCHED_THREAD_SETPRIORITY */
typedef struct {
    pthread_t thread_id;    /*!< Thread id */
    osPriority priority;    /*!< Thread priority */
} ds_osSetPriority_t;

/** Argument struct for SYSCALL_SCHED_SIGNAL_SET
 *  and KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR */
typedef struct {
    pthread_t thread_id;   /*!< Thread id */
    int32_t signal;         /*!< Thread signals to set */
} ds_osSignal_t;

/** Argument struct for SYSCALL_SCHED_SIGNAL_WAIT */
typedef struct {
    int32_t signals;        /*!< Thread signal(s) to wait */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osSignalWait_t;

/** Argument struct for SYSCALL_SEMAPHORE_WAIT */
typedef struct {
    uint32_t * s;           /*!< Pointer to the semaphore */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osSemaphoreWait_t;

#endif /* SYSCALLDEF_H */
