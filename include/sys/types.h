/**
 *******************************************************************************
 * @file    pthread.h
 * @author  Olli Vanhoja
 * @brief   Threads.
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
#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef size_t blkcnt_t; /*!< Used for file block counts. */
typedef int blksize_t; /*!< Used for block sizes. */
typedef uint32_t clock_t; /*!< Used for system times in clock ticks
                            or CLOCKS_PER_SEC. */
typedef int clockid_t; /*!< Used for clock ID type in the clock and
                         timer functions. */
typedef uint32_t dev_t; /*!< Device identifier */
typedef size_t fsblkcnt_t; /*!< Used for file system block counts. */
typedef size_t fsfilcnt_t; /*!< Used for file system file counts. */
typedef int uid_t; /*!< Used for user IDs. */
typedef int gid_t; /*!< Used for group IDs. */
typedef int id_t; /*!< Used as a general identifier; can be used to contain at
                    least a pid_t, uid_t, or gid_t. */
typedef uint64_t ino_t; /*!< Used for file serial numbers.*/
typedef uint32_t key_t; /*!< Used for XSI interprocess communication. */
typedef int mode_t; /*!< Used for some file attributes. */
typedef int nlink_t; /*!< Used for link counts. */
typedef size_t off_t; /*!< Used for file sizes. */
typedef int pid_t; /*!< Process ID. */
#ifndef ssize_t
#ifndef SSIZE_MAX
#define SSIZE_MAX INT32_MAX
typedef int32_t ssize_t; /*!< Used for a count of bytes or an error indication. */
#endif
#endif
typedef uint64_t time_t; /*!< Used for time in seconds. */
typedef uint64_t useconds_t; /*!< Used for time in microseconds. */
typedef uint64_t suseconds_t; /*!< Used for time in microseconds */
typedef int timer_t; /*!< Used for timer ID returned by timer_create(). */


/* TODO Missing types:
 * - trace_attr_t
 * - trace_event_id_t
 * - trace_event_set_t
 * - trace_id_t
 * -
 */

/* TODO Below this is some legacy */

/* TODO This should be internal to kernel */
/// Status code values returned by CMSIS-RTOS functions
typedef enum  {
    osOK                    =     0,        ///< function completed; no event occurred.
    osEventSignal           =  0x08,        ///< function completed; signal event occurred.
    osEventMessage          =  0x10,        ///< function completed; message event occurred.
    osEventMail             =  0x20,        ///< function completed; mail event occurred.
    osEventTimeout          =  0x40,        ///< function completed; timeout occurred.
    osErrorParameter        =  0x80,        ///< parameter error: a mandatory parameter was missing or specified an incorrect object.
    osErrorResource         =  0x81,        ///< resource not available: a specified resource was not available.
    osErrorTimeoutResource  =  0xC1,        ///< resource not available within given time: a specified resource was not available within the timeout period.
    osErrorISR              =  0x82,        ///< not allowed in ISR context: the function cannot be called from interrupt service routines.
    osErrorISRRecursive     =  0x83,        ///< function called multiple times from ISR with same object.
    osErrorPriority         =  0x84,        ///< system cannot determine priority or thread has illegal priority.
    osErrorNoMemory         =  0x85,        ///< system is out of memory: it was impossible to allocate or reserve memory for the operation.
    osErrorValue            =  0x86,        ///< value of a parameter is out of range.
    osErrorOS               =  0xFF,        ///< unspecified RTOS error: run-time error but no other error message fits.
    os_status_reserved      =  0x7FFFFFFF   ///< prevent from enum down-size compiler optimization.
} osStatus;

/**
 * Timeout value
 */
#define osWaitForever     0x0u       /*!< wait forever timeout value */

/// Timer type value for the timer definition
typedef enum  {
  osTimerOnce             =     0,       ///< one-shot timer
  osTimerPeriodic         =     1        ///< repeating timer
} os_timer_type;

/// Timer ID identifies the timer (pointer to a timer control block).
typedef int osTimerId;

/**
 * Semaphore ID identifies the semaphore (pointer to a semaphore control block).
 * @note All data related to the mutex is stored in user space structure and
 *       it is DANGEROUS to edit its contents in thread context.
 */
typedef struct os_semaphore_cb osSemaphore;

/// Message ID identifies the message queue (pointer to a message queue control block).
typedef struct os_messageQ_cb *osMessageQId;

/// Mail ID identifies the mail queue (pointer to a mail queue control block).
typedef struct os_mailQ_cb *osMailQId;

/// Event structure contains detailed information about an event.
typedef struct  {
    osStatus                 status;    ///< status code: event or error information
    union  {
        uint32_t                    v;  ///< message as 32-bit value
        void                       *p;  ///< message or mail as void pointer
        int32_t               signals;  ///< signal flags
    } value;                            ///< event value
    union  {
        osMailQId             mail_id;  ///< mail id obtained by \ref osMailCreate
        osMessageQId       message_id;  ///< message id obtained by \ref osMessageCreate
    } def;                              ///< event definition
} osEvent;

#endif /* TYPES_H */
