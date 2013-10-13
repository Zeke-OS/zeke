/**
 *******************************************************************************
 * @file    locks.c
 * @author  Olli Vanhoja
 * @brief   Locks and Syscall handlers for locks
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

#include <syscall.h>
#include <semaphore.h>
#ifndef PU_TEST_BUILD
#include "hal/hal_core.h"
#endif
#include "sched.h"
#include "timers.h"
#include "locks.h"

static int locks_semaphore_thread_spinwait(uint32_t * s, uint32_t millisec);

/** @addtogroup Internal_Locks
  * @{
  */

/** @addtogroup Semaphore
  * @{
  */

/**
 * Increment a semaphore
 * @param s a semaphore.
 */
void locks_semaphore_v(uint32_t * s)
{
    *s += 1;
}

/**
 * Try to decrease a semaphore s
 * @param s a semaphore.
 * @return 0 if decrease failed; otherwise succeed.
 */
int locks_semaphore_p(uint32_t * s)
{
    if (*s > 0) {
        *s -= 1;
        return 1;
    }
    return 0;
}

/**
 * Wait until a Semaphore token becomes available
 * @param s semaphore.
 * @return number of available tokens, or error code defined above.
 */
static int locks_semaphore_thread_spinwait(uint32_t * s, uint32_t millisec)
{
    if (current_thread->wait_tim >= 0) {
        if (timers_get_owner(current_thread->wait_tim) < 0) {
            /* Timeout */
            return OS_SEMAPHORE_THREAD_SPINWAIT_RES_ERROR;
        }
    }

    /* Try */
    if (!locks_semaphore_p(s)) {
        if (current_thread->wait_tim < 0) {
            /* Get a timer for timeout */
            if ((current_thread->wait_tim = timers_add(current_thread->id,
                            TIMERS_FLAG_ENABLED, millisec)) < 0) {
                /* Resource Error: No free timers */
                return OS_SEMAPHORE_THREAD_SPINWAIT_RES_ERROR;
            }
        } /* else timer is already ticking for the thread. */

        /* Still waiting for a semaphore token */
        return OS_SEMAPHORE_THREAD_SPINWAIT_WAITING;
    }

    /* Release the previously initialized wait_tim */
    if (current_thread->wait_tim >= 0) {
        timers_release(current_thread->wait_tim);
    }

    return (int)*s;
}

/**
  * @}
  */

uint32_t locks_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_MUTEX_TEST_AND_SET:
        return (uint32_t)test_and_set((int *)p);

    case SYSCALL_SEMAPHORE_WAIT:
        return (uint32_t)locks_semaphore_thread_spinwait(
                ((ds_osSemaphoreWait_t *)(p))->s,
                ((ds_osSemaphoreWait_t *)(p))->millisec);

    case SYSCALL_SEMAPHORE_RELEASE:
        if (((os_semaphore_cb_t *)(p))->s < ((os_semaphore_cb_t *)(p))->count)
            locks_semaphore_v(&(((os_semaphore_cb_t *)(p))->s));
        return (uint32_t)NULL;

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */
