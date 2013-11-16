/**
 *******************************************************************************
 * @file    kernel.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
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

/** @addtogroup Library_Functions
  * @{
  */

#include <hal/hal_core.h>
#include "syscalldef.h"
#include "syscall.h"
#include <kernel.h>

void osGetLoadAvg(uint32_t loads[3])
{
    syscall(SYSCALL_SCHED_GET_LOADAVG, loads);
}

/** @addtogroup Thread_Management
  * @{
  */

osStatus osThreadTerminate(pthread_t thread_id)
{
    return (osStatus)syscall(SYSCALL_SCHED_THREAD_TERMINATE, &thread_id);
}

osStatus osThreadYield(void)
{
    /* TODO Should use temp reschedule before this */

    /* Request immediate context switch */
    req_context_switch();

    return (osStatus)osOK;
}

osStatus osThreadSetPriority(pthread_t thread_id, osPriority priority)
{
    ds_osSetPriority_t ds = {
        .thread_id = thread_id,
        .priority = priority
    };

    return (osStatus)syscall(SYSCALL_SCHED_THREAD_SETPRIORITY, &ds);
}

osPriority osThreadGetPriority(pthread_t thread_id)
{
    return (osPriority)syscall(SYSCALL_SCHED_THREAD_GETPRIORITY, &thread_id);
}

int __error(void)
{
    return (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL);
}


/**
  * @}
  */

/** @addtogroup Wait
  * @{
  */

/* Generic Wait Functions ****************************************************/

osStatus osDelay(uint32_t millisec)
{
    osStatus result;

    result = (osStatus)syscall(SYSCALL_SCHED_DELAY, &millisec);

    if (result != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    return result;
}

osEvent osWait(uint32_t millisec)
{
    osEvent result;

    result.status = (osStatus)syscall(SYSCALL_SCHED_WAIT, &millisec);

    if (result.status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    syscall(SYSCALL_SCHED_EVENT_GET, &result);
    return result;
}

/**
  * @}
  */

/** @addtogroup Signal
  * @{
  */

/* Signal Management *********************************************************/

int32_t osSignalSet(pthread_t thread_id, int32_t signal)
{
    ds_osSignal_t ds = {
        .thread_id = thread_id,
        .signal    = signal
    };

    return (int32_t)syscall(SYSCALL_SIGNAL_SET, &ds);
}

int32_t osSignalClear(pthread_t thread_id, int32_t signal)
{
    ds_osSignal_t ds = {
        .thread_id = thread_id,
        .signal    = signal
    };

    return (int32_t)syscall(SYSCALL_SIGNAL_CLEAR, &ds);
}

int32_t osSignalGetCurrent(void)
{
    return (int32_t)syscall(SYSCALL_SIGNAL_GETCURR, NULL);
}

int32_t osSignalGet(pthread_t thread_id)
{
    return (int32_t)syscall(SYSCALL_SIGNAL_GET, &thread_id);
}

osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
    ds_osSignalWait_t ds = {
        .signals  = signals,
        .millisec = millisec
    };
    osEvent result;

    result.status = (osStatus)syscall(SYSCALL_SIGNAL_WAIT, &ds);

    if (result.status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    syscall(SYSCALL_SCHED_EVENT_GET, &result);
    return result;
}

/**
  * @}
  */

/** @addtogroup Mutex
  * @{
  */

/* Mutex Management ***********************************************************
 * NOTE: osMutexId/mutex_id is a direct pointer to a os_mutex_cb structure.
 */

/** TODO implement sleep strategy */

osMutex osMutexCreate(osMutexDef_t *mutex_def)
{
    mutex_cb_t cb = {
        .thread_id = -1,
        .lock      = 0,
        .strategy  = mutex_def->strategy
    };

    return cb;
}

osStatus osMutexWait(osMutex * mutex, uint32_t millisec)
{
    if (millisec != 0) {
        /** TODO Only spinlock is supported at the moment; implement timeout */
        return osErrorParameter;
    }

    while (syscall(SYSCALL_MUTEX_TEST_AND_SET, (void*)(&(mutex->lock)))) {
        /** TODO Should we lower the priority until lock is acquired
         *       osThreadGetPriority & osThreadSetPriority */
        /* TODO reschedule call in kernel space? see semaphorewait */
        /* Reschedule while waiting for lock */
        req_context_switch(); /* This should be done in user space */
    }

    mutex->thread_id = pthread_self();
    return osOK;
}

osStatus osMutexRelease(osMutex * mutex)
{
    if (mutex->thread_id == pthread_self()) {
        mutex->lock = 0;
        return osOK;
    }
    return osErrorResource;
}

/**
  * @}
  */

/** @addtogroup Semaphore
  * @{
  */

//osSemaphore osSemaphoreCreate(osSemaphoreDef_t * semaphore_def, int32_t count)
//{
    /* TODO Implementation */
//}

int32_t osSemaphoreWait(osSemaphore * semaphore, uint32_t millisec)
{
    ds_osSemaphoreWait_t ds = {
        .s = &(semaphore->s),
        .millisec = millisec
    };
    int retVal;

    /* Loop between kernel mode and thread mode :) */
    while ((retVal = syscall(SYSCALL_SEMAPHORE_WAIT, &ds)) < 0) {
        if (retVal == OS_SEMAPHORE_THREAD_SPINWAIT_RES_ERROR) {
            return -1;
        }

        /* TODO priority should be lowered or some resceduling should be done
         * in the kernel so this loop would not waste time before automatic
         * rescheduling. */
        req_context_switch();
    }

    return retVal;
}

osStatus osSemaphoreRelease(osSemaphore * semaphore)
{
    syscall(SYSCALL_SEMAPHORE_RELEASE, semaphore);
    return (osStatus)osOK;
}

/**
  * @}
  */

/**
  * @}
  */
