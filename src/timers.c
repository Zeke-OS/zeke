/**
 *******************************************************************************
 * @file    timers.c
 * @author  Olli Vanhoja
 *
 * @brief   Kernel timers
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

#include <autoconf.h>
#include <kinit.h>
#include <sched.h>
#include "timers.h"

/* Internal Flag Bits */
/* NONE */

/**
 * Indicates a free timer position on thread_id of allocation entry.
 */
#define TIMERS_POS_FREE -1

/** Timer allocation struct */
typedef struct {
    timers_flags_t flags;   /*!< Timer flags
                             * + 0 = Timer state
                             *     + 0 = disabled
                             *     + 1 = enabled
                             * + 1 = Timer type
                             *     + 0 = one-shot
                             *     + 1 = periodic
                             */
    pthread_t thread_id;   /*!< Thread id */
    uint32_t reset_val;     /*!< Reset value for repeating timer */
    uint32_t expires;       /*!< Timer expiration time */
} timer_alloc_data_t;

uint32_t timers_value; /*!< Current tick value */
static volatile timer_alloc_data_t timers_array[configTIMERS_MAX];

static mtx_t timers_lock;

void timers_init(void) __attribute__((constructor));
static int timers_calc_exp(int millisec);

void timers_init(void)
{
    SUBSYS_INIT();

    mtx_init(&timers_lock, MTX_DEF | MTX_SPIN);

    timers_value = 0;
    for (int i = 0; i < configTIMERS_MAX; i++) {
        timers_array[i].flags = 0;
        timers_array[i].thread_id = TIMERS_POS_FREE;
    }

    SUBSYS_INITFINI("Kernel timers init OK");
}

void timers_run(void)
{
    int i;
    uint32_t value;

    timers_value++;
    value = timers_value;
    i = 0;
    do {
        uint32_t exp = timers_array[i].expires;
        if (timers_array[i].flags & TIMERS_FLAG_ENABLED) {
            if (exp == value) {
                threadInfo_t * thread;
                thread = sched_get_pThreadInfo(timers_array[i].thread_id);
                if (thread) {
                    thread->wait_tim = -1;
                    sched_thread_set_exec(thread->id);
                }

                if (!(timers_array[i].flags & TIMERS_FLAG_PERIODIC)) {
                    /* Release the timer */
                    timers_array[i].flags &= ~TIMERS_FLAG_ENABLED;
                    timers_array[i].thread_id = TIMERS_POS_FREE;
                } else {
                    /* Repeating timer */
                    timers_array[i].expires = timers_calc_exp(timers_array[i].reset_val);
                }
            }
        }
    } while (++i < configTIMERS_MAX);
}

/**
 * Allocate a new timer
 * @param thread_id thread id to add this timer for.
 * @param flags User modifiable flags (see: TIMERS_USER_FLAGS)-
 * @param millisec delay to trigger from the time when enabled.
 * @param return -1 if allocation failed.
 */
int timers_add(pthread_t thread_id, timers_flags_t flags, uint32_t millisec)
{
    int i = 0;
    flags &= TIMERS_USER_FLAGS; /* Allow only user flags to be set */

    mtx_spinlock(&timers_lock);
    do { /* Locate first free timer */
        if (timers_array[i].thread_id == TIMERS_POS_FREE) {
            timers_array[i].thread_id = thread_id; /* reserve */

            if (flags & TIMERS_FLAG_PERIODIC) { /* Periodic timer */
                timers_array[i].reset_val = millisec;
            }

            if (millisec > 0) {
                timers_array[i].expires = timers_calc_exp(millisec);
            }

            timers_array[i].flags = flags; /* enable */
            mtx_unlock(&timers_lock);
            return i;
        }
    } while (++i < configTIMERS_MAX);
    mtx_unlock(&timers_lock);

    return -1;
}

void timers_start(int tim)
{
    if (tim < configTIMERS_MAX && tim >= 0)
        return;

    timers_array[tim].flags |= TIMERS_FLAG_ENABLED;
}

/* TODO should check that owner matches before relasing the timer
 */
void timers_release(int tim)
{
    if (tim < configTIMERS_MAX && tim >= 0)
        return;

    /* Release the timer */
    timers_array[tim].flags = 0;
    timers_array[tim].thread_id = TIMERS_POS_FREE;
}

pthread_t timers_get_owner(int tim) {
    pthread_t retval;

    if (tim < configTIMERS_MAX && tim >= 0)
        return -1;

    mtx_spinlock(&timers_lock);
    retval = timers_array[tim].thread_id;
    mtx_unlock(&timers_lock);

    return retval;
}

static int timers_calc_exp(int millisec)
{
    int exp;
    uint32_t value = timers_value;

    exp = value + ((millisec * configSCHED_HZ) / 1000);
    if (exp == value)
        exp++;

    return exp;
}
