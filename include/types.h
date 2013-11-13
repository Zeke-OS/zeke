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

/**
 * Thread id.
 */
typedef int pthread_t;

/**
 * Entry point of a thread.
 */
typedef void (*os_pthread) (void const * argument);

/* Process ID */
typedef int pid_t;

/* TODO Below this is some legacy */

#include <mutex.h>

/**
 * Priority used for thread control.
 */
typedef enum {
    osPriorityIdle          = -3,       ///< priority: idle (lowest)
    osPriorityLow           = -2,       ///< priority: low
    osPriorityBelowNormal   = -1,       ///< priority: below normal
    osPriorityNormal        =  0,       ///< priority: normal (default)
    osPriorityAboveNormal   = +1,       ///< priority: above normal
    osPriorityHigh          = +2,       ///< priority: high
    osPriorityRealtime      = +3,       ///< priority: realtime (highest)
    osPriorityError         =  0x84     ///< system cannot determine priority or thread has illegal priority
} osPriority;

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
 * Mutex cb mutex.
 * @note All data related to the mutex is stored in user space structure and
 *       it is DANGEROUS to edit its contents in thread context.
 */
typedef struct os_mutex_cb osMutex;

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

/// Mutex Definition structure contains setup information for a mutex.
typedef const struct os_mutex_def  {
    enum os_mutex_strategy strategy;
} osMutexDef_t;

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
