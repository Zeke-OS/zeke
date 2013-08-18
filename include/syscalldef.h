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
    osThreadDef_t * def;    /*!< Thread definitions for a new thread */
    void * argument;        /*!< Thread parameter(s) pointer */
} ds_osThreadCreate_t;

/** Argument struct for SYSCALL_SCHED_THREAD_SETPRIORITY */
typedef struct {
    osThreadId thread_id;   /*!< Thread id */
    osPriority priority;    /*!< Thread priority */
} ds_osSetPriority_t;

/** Argument struct for SYSCALL_SCHED_SIGNAL_SET
 *  and KERNEL_SYSCALL_SCHED_SIGNAL_CLEAR */
typedef struct {
    osThreadId thread_id;   /*!< Thread id */
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

#if configDEVSUBSYS != 0
/** Argument struct for some dev syscalls */
typedef struct {
    osDev_t dev;            /*!< Device */
    osThreadId thread_id;   /*!< Thread id */
} ds_osDevHndl_t;

/** Argument struct for dev write syscalls */
typedef struct {
    osDev_t dev;            /*!< Device to be read from or written to. */
    void * data;            /*!< Data pointer. */
} ds_osDevCData_t;

/** Generic argument struct for block functions in dev subsystem */
typedef struct {
    void * buff;            /*!< pointer to a block of memory with a size of at
                             *   least (size*count) bytes, converted to a
                             *   void *. */
    size_t size;            /*!< in bytes, of each element. */
    size_t count;           /*!< number of elements, each one with a size of
                             *   size bytes. */
    osDev_t dev;            /*!< device to be read from or written to. */
} ds_osDevBData_t;

/** Argument struct for block seek function in dev subsystem */
typedef struct {
    int offset;             /*!< Number of size units to offset from origin. */
    int origin;             /*!< Position used as reference for the offset. */
    size_t size;            /*!< in bytes, of each element. */
    osDev_t dev;            /*!< device to be seeked from. */
} ds_osDevBSeekData_t;

/** Argument struct for SYSCALL_DEV_WAIT */
typedef struct {
    osDev_t dev;            /*!< Device */
    uint32_t millisec;      /*!< Timeout in ms */
} ds_osDevWait_t;
#endif

#endif /* SYSCALLDEF_H */
