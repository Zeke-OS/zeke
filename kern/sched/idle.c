/**
 *******************************************************************************
 * @file    idle.c
 * @author  Olli Vanhoja
 * @brief   Kernel idle thread and idle coroutine management.
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

/* TODO Idle thread/sched is not MP safe. */

#include <errno.h>
#include <buf.h>
#include <hal/core.h>
#include <kerror.h>
#include <ksched.h>
#include <idle.h>

SET_DECLARE(_idle_tasks, struct _idle_task_desc);

struct thread_info * idle_info;

void * idle_thread(void * arg)
{
    struct _idle_task_desc ** desc_p;

    while (1) {
        /* Execute idle coroutines */
        SET_FOREACH(desc_p, _idle_tasks) {
            struct _idle_task_desc * desc = *desc_p;

            desc->fn(desc->arg);
        }

        idle_sleep();
    }
}

static int idle_insert(struct scheduler * sobj, struct thread_info * thread)
{
    if (idle_info)
        return -ENOTSUP;

    thread_flags_set(thread, SCHED_INTERNAL_FLAG);
    idle_info = thread;

    return 0;
}

static struct thread_info * idle_schedule(struct scheduler * sobj)
{
    return idle_info;
}

static unsigned get_nr_active(struct scheduler * sobj)
{
    return 0;
}

static struct scheduler sched_idle = {
    .name = "sched_idle",
    .insert = idle_insert,
    .run = idle_schedule,
    .get_nr_active_threads = get_nr_active,
};

struct scheduler * sched_create_idle(void)
{
    struct _sched_pthread_create_args tdef_idle;
    struct buf * bp;

    bp = geteblk(MMU_PGSIZE_COARSE);

    tdef_idle = (struct _sched_pthread_create_args){
        .param.sched_policy   = SCHED_OTHER + 1,
        .param.sched_priority = NZERO,
        .stack_addr = (void *)bp->b_data,
        .stack_size = bp->b_bufsize,
        .flags      = 0,
        .start      = idle_thread,
        .arg1       = 0,
        .del_thread = NULL,
    };

    thread_create(&tdef_idle, 1);

    return &sched_idle;
}
