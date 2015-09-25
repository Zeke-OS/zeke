/**
 *******************************************************************************
 * @file    timers.c
 * @author  Olli Vanhoja
 * @brief   Kernel timers
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/* TODO MP version, per CPU timers */

#include <sys/linker_set.h>
#include <machine/atomic.h>
#include <hal/hw_timers.h>
#include <kinit.h>
#include <thread.h>
#include <timers.h>

/** Timer allocation struct */
struct timer_cb {
    atomic_t flags;             /*!< Timer flags:
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

static struct timer_cb timers_array[configTIMERS_MAX];
#define VALID_TIMER_ID(x) ((x) < configTIMERS_MAX && (x) >= 0)

int __kinit__ timers_init(void)
{
    SUBSYS_INIT("timers");

    for (size_t i = 0; i < configTIMERS_MAX; i++) {
        timers_array[i].flags = ATOMIC_INIT(0);
    }

    return 0;
}

void timers_run(void)
{
    size_t i = 0;
    uint64_t now = get_utime();
    const int enflags = TIMERS_FLAG_INUSE | TIMERS_FLAG_ENABLED;

    do {
        struct timer_cb * const timer = &timers_array[i];
        timers_flags_t flags = atomic_read(&timer->flags);

        if ((flags & enflags) == enflags) {
            if ((now - timer->start) >= timer->interval) {
                timer->event_fn(timer->event_arg);

                if (!(flags & TIMERS_FLAG_PERIODIC)) {
                    /* Stop the timer */
                    timer->flags &= ~TIMERS_FLAG_ENABLED;
                } else {
                    /* Repeating timer */
                    timer->start = get_utime();
                }
            }
        }
    } while (++i < configTIMERS_MAX);
}
DATA_SET(pre_sched_tasks, timers_run);

int timers_add(void (*event_fn)(void *), void * event_arg,
               timers_flags_t flags, uint64_t usec)
{
    struct timer_cb * timer;
    size_t i = 0;

    flags &= TIMERS_EXT_FLAGS; /* Allow only external flags to be set */

    do { /* Locate first free timer */
        timer = &timers_array[i];

        if (atomic_cmpxchg(&timer->flags, 0, flags) == 0) {
            timer->event_fn = event_fn;
            timer->event_arg = event_arg;
            timer->interval = usec;
            timer->start = get_utime();
            flags |= TIMERS_FLAG_INUSE;
            atomic_set(&timer->flags, flags);

            return i;
        }
    } while (++i < configTIMERS_MAX);

    return TMNOVAL;
}

int64_t timers_get_split(int tim)
{
    uint64_t now;
    struct timer_cb * timer;

    if (!VALID_TIMER_ID(tim))
        return -1;

    now = get_utime();
    timer = &timers_array[tim];

    return now - timer->start;
}

void timers_start(int tim)
{
    if (!VALID_TIMER_ID(tim))
        return;

    atomic_add(&timers_array[tim].flags, TIMERS_FLAG_ENABLED);
}

void timers_stop(int tim)
{
    if (!VALID_TIMER_ID(tim))
        return;

    atomic_sub(&timers_array[tim].flags, TIMERS_FLAG_ENABLED);
}

void timers_release(int tim)
{
    if (!VALID_TIMER_ID(tim))
        return;

    atomic_set(&timers_array[tim].flags, 0);
}
