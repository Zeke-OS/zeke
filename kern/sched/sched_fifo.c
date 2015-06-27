/**
 *******************************************************************************
 * @file    sched_fifo.c
 * @author  Olli Vanhoja
 * @brief   FIFO scheduler.
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
#include <sys/tree.h>
#include <kerror.h>
#include <kmalloc.h>
#include <libkern.h>
#include <ksched.h>

#define SCHED_POLFLAG_INFIFORQ  0x01 /*!< Thread in run queue. */

#define FIFORUNQ_ENTRY  sched.fifo.runq_entry_

struct sched_fifo {
    struct scheduler sched;
    unsigned nr_active;
    RB_HEAD(fiforunq, thread_info) runq_head;
};

static int fifo_prio_compare(struct thread_info * a, struct thread_info * b)
{
    return a->sched.fifo.prio - b->sched.fifo.prio;
}

RB_PROTOTYPE_STATIC(fiforunq, thread_info, FIFORUNQ_ENTRY, fifo_prio_compare);
RB_GENERATE_STATIC(fiforunq, thread_info, FIFORUNQ_ENTRY, fifo_prio_compare);

static int fifo_insert(struct scheduler * sobj, struct thread_info * thread)
{
    struct sched_fifo * fifo = container_of(sobj, struct sched_fifo, sched);

    if (!SCHED_TEST_POLFLAG(thread, SCHED_POLFLAG_INFIFORQ)) {
        RB_INSERT(fiforunq, &fifo->runq_head, thread);
        thread->sched.ts_counter = -1; /* Not used. */

        /*
         * The priority of a process is static until it's removed from the queue
         * and it can only change on reinsert.
         */
        thread->sched.fifo.prio = thread->param.sched_priority;

        thread->sched.policy_flags |= SCHED_POLFLAG_INFIFORQ;
        fifo->nr_active++;
    } else {
        /* Reinsert should update the priority. */
        RB_REMOVE(fiforunq, &fifo->runq_head, thread);
        thread->sched.fifo.prio = thread->param.sched_priority;
        RB_INSERT(fiforunq, &fifo->runq_head, thread);
    }

    return 0;
}

static void fifo_remove(struct scheduler * sobj, struct thread_info * thread)
{
    struct sched_fifo * fifo = container_of(sobj, struct sched_fifo, sched);

    if (SCHED_TEST_POLFLAG(thread, SCHED_POLFLAG_INFIFORQ)) {
        RB_REMOVE(fiforunq, &fifo->runq_head, thread);
        thread->sched.policy_flags &= ~SCHED_POLFLAG_INFIFORQ;
        fifo->nr_active--;
    }
}

static struct thread_info * fifo_schedule(struct scheduler * sobj)
{
    struct sched_fifo * fifo = container_of(sobj, struct sched_fifo, sched);
    struct thread_info * thread;

    if (RB_EMPTY(&fifo->runq_head))
        return NULL;

    while ((thread = RB_MIN(fiforunq, &fifo->runq_head))) {
        const enum thread_state state = thread_state_get(thread);

        switch (state) {
        case THREAD_STATE_READY:
            fifo_remove(sobj, thread);
            break;
        case THREAD_STATE_EXEC:
            return thread; /* select */
        case THREAD_STATE_DEAD:
            fifo_remove(sobj, thread);
            if (thread_flags_is_set(thread, SCHED_DETACH_FLAG))
                thread_remove(thread->id);
            break;
        default:
             KERROR(KERROR_ERR, "Thread (%d) state: %d\n", thread->id, state);
             panic("Inconsistent thread state");
        }
    }

    return NULL;
}

static unsigned get_nr_active(struct scheduler * sobj)
{
    struct sched_fifo * fifo = container_of(sobj, struct sched_fifo, sched);

    return fifo->nr_active;
}

/**
 * Initializer struct for a fifo scheduler.
 */
static const struct sched_fifo sched_fifo_init = {
    .sched.name = "sched_fifo",
    .sched.insert = fifo_insert,
    .sched.run = fifo_schedule,
    .sched.get_nr_active_threads = get_nr_active,
};

struct scheduler * sched_create_fifo(void)
{
    struct sched_fifo * sched;

    sched = kmalloc(sizeof(struct sched_fifo));
    if (!sched)
        return NULL;

    *sched = sched_fifo_init; /* init */
    RB_INIT(&sched->runq_head);

    return &sched->sched;
}
