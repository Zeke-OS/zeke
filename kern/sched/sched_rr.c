/**
 *******************************************************************************
 * @file    sched_rr.c
 * @author  Olli Vanhoja
 * @brief   RR scheduler.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stddef.h>
#include <ksched.h>

#define RR_POLF_INRRRQ      0x01    /*!< Thread in rr run queue. */

/**
 * Test if policy flag is set.
 */
#define TEST_POLF(_thread_, _flag_) \
    ((thread->sched.policy_flags & _flag_) == _flag_)

#define RRRUNQ_ENTRY        sched.rrrunq_entry_

TAILQ_HEAD(runq, thread_info) runq_head =
    TAILQ_HEAD_INITIALIZER(runq_head);

static int rr_insert(struct thread_info * thread)
{
    if (!TEST_POLF(thread, RR_POLF_INRRRQ)) {
        TAILQ_INSERT_TAIL(&runq_head, thread, RRRUNQ_ENTRY);
        thread->sched.ts_counter = 1; /* TODO */
        thread->sched.policy_flags |= RR_POLF_INRRRQ;
    }

    return 0;
}

static void rr_remove(struct thread_info * thread)
{
    if (TEST_POLF(thread, RR_POLF_INRRRQ)) {
        TAILQ_REMOVE(&runq_head, thread, RRRUNQ_ENTRY);
        thread->sched.policy_flags &= ~RR_POLF_INRRRQ;
    }
}

static void rr_thread_act(struct thread_info * thread)
{
    const enum thread_state state = thread_state_get(thread);

    switch (state) {
    case THREAD_STATE_READY:
        /* Thread already in readyq */
        rr_remove(thread);
        break;
    case THREAD_STATE_EXEC:
        if (thread->sched.ts_counter <= 0) {
            thread->sched.ts_counter = 1; /* TODO */
        }
        break;
    case THREAD_STATE_BLOCKED:
        rr_remove(thread);
        break;
    case THREAD_STATE_DEAD:
        rr_remove(thread);
        if (thread_flags_is_set(thread, SCHED_DETACH_FLAG))
            thread_remove(thread->id);
        break;
    default:
        KERROR(KERROR_ERR, "Thread (%d) state: %d\n", thread->id, state);
        panic("Inconsistent thread state");
    }
}

static void rr_schedule(void)
{
    struct thread_info * next;
    struct thread_info * tmp;

    TAILQ_FOREACH_SAFE(next, &runq_head, RRRUNQ_ENTRY, tmp) {
        if (sched_csw_ok(next)) {
            current_thread = next;
            return;
        } else {
            rr_thread_act(next);
        }
    }
}

struct scheduler sched_rr = {
    .name = "sched_rr",
    .insert = rr_insert,
    .run = rr_schedule,
};
