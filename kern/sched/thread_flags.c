/**
 *******************************************************************************
 * @file    thread_flags.h
 * @author  Olli Vanhoja
 * @brief   Manipulate thread flags.
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

#include <hal/core.h>
#include <kerror.h>
#include <thread.h>

int thread_flags_set(struct thread_info * thread, uint32_t flags_mask)
{
    KASSERT(thread != NULL, "thread must be set");

    mtx_lock(&thread->sched.tdlock);
    thread->flags |= flags_mask;
    mtx_unlock(&thread->sched.tdlock);

    return 0;
}

int thread_flags_clear(struct thread_info * thread, uint32_t flags_mask)
{
    KASSERT(thread != NULL, "thread must be set");

    mtx_lock(&thread->sched.tdlock);
    thread->flags &= ~flags_mask;
    mtx_unlock(&thread->sched.tdlock);

    return 0;
}

uint32_t thread_flags_get(struct thread_info * thread)
{
    uint32_t flags;

    KASSERT(thread != NULL, "thread must be set");

    mtx_lock(&thread->sched.tdlock);
    flags = thread->flags;
    mtx_unlock(&thread->sched.tdlock);

    return flags;
}

int thread_flags_is_set(struct thread_info * thread, uint32_t flags_mask)
{
    uint32_t flags;

    flags = thread_flags_get(thread);
    return (flags & flags_mask) == flags_mask;
}

int thread_flags_not_set(struct thread_info * thread, uint32_t flags_mask)
{
    uint32_t flags;

    flags = thread_flags_get(thread);
    return (flags & flags_mask) == 0;
}

enum thread_state thread_state_get(struct thread_info * thread)
{
    enum thread_state state;

    mtx_lock(&thread->sched.tdlock);
    state = thread->sched.state;
    mtx_unlock(&thread->sched.tdlock);

    return state;
}

enum thread_state thread_state_set(struct thread_info * thread,
                                   enum thread_state state)
{
    enum thread_state old_state;

    mtx_lock(&thread->sched.tdlock);
    old_state = thread->sched.state;
    if (old_state != THREAD_STATE_DEAD)
        thread->sched.state = state;
    mtx_unlock(&thread->sched.tdlock);

    return old_state;
}
