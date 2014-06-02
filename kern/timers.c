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
#include <sys/linker_set.h>
#include <sched.h>
#include <timers.h>

/** Timer allocation struct */
struct timer_cb {
    timers_flags_t flags;       /*!< Timer flags:
                                 * + 0 = Timer state
                                 *     + 0 = disabled
                                 *     + 1 = enabled
                                 * + 1 = Timer type
                                 *     + 0 = one-shot
                                 *     + 1 = periodic
                                 */
    void (*event_fn)(void *);   /*!< Event handler for the timer. */
    void * event_arg;           /*!< Argument for event handler. */
    uint64_t interval;          /*!< Timer interval. */
    uint64_t start;             /*!< Timer start value. */
};

/*
 * Lock to be used between threads accessing timer data structures. Scheduler
 * should not try to spin on this lock as it would hang the kernel.
 */
static mtx_t timers_lock;
static volatile struct timer_cb timers_array[configTIMERS_MAX];
#define VALID_TIMER_ID(x) ((x) < configTIMERS_MAX && (x) >= 0)

void timers_init(void) __attribute__((constructor));

void timers_init(void)
{
    SUBSYS_INIT();

    mtx_init(&timers_lock, MTX_DEF | MTX_SPIN);

    SUBSYS_INITFINI("timers OK");
}

void timers_run(void)
{
    size_t i = 0;
    uint64_t now = get_utime();

    /* TODO Not MP safe? */
    do {
        if (timers_array[i].flags & TIMERS_FLAG_ENABLED) {
            if ((now - timers_array[i].start) >= timers_array[i].interval) {
                timers_array[i].event_fn(timers_array[i].event_arg);

                if (!(timers_array[i].flags & TIMERS_FLAG_PERIODIC)) {
                    /* Stop the timer */
                    timers_array[i].flags &= ~TIMERS_FLAG_ENABLED;
                } else {
                    /* Repeating timer */
                    timers_array[i].start = get_utime();
                }
            }
        }
    } while (++i < configTIMERS_MAX);
}
DATA_SET(pre_sched_tasks, timers_run);

int timers_add(void (*event_fn)(void *), void * event_arg,
        timers_flags_t flags, uint64_t usec)
{
    size_t i = 0;
    int retval = -1;

    flags &= TIMERS_EXT_FLAGS; /* Allow only external flags to be set */

    mtx_spinlock(&timers_lock);
    do { /* Locate first free timer */
        if (timers_array[i].event_fn == 0) {
            timers_array[i].event_fn = event_fn; /* reserve */
            timers_array[i].event_arg = event_arg;
            timers_array[i].interval = usec;
            timers_array[i].start = get_utime();
            timers_array[i].flags = flags; /* enable */

            retval = i;
            break;
        }
    } while (++i < configTIMERS_MAX);
    mtx_unlock(&timers_lock);

    return retval;
}

void timers_start(int tim)
{
    if (!VALID_TIMER_ID(tim))
        return;

    timers_array[tim].flags |= TIMERS_FLAG_ENABLED;
}

void timers_release(int tim)
{
    if (!VALID_TIMER_ID(tim))
        return;

    /* Release the timer */
    timers_array[tim].flags = 0;
    timers_array[tim].event_fn = 0;
}
