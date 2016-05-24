/**
 *******************************************************************************
 * @file    kthread.c
 * @author  Olli Vanhoja
 * @brief   Kernel thread management.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <buf.h>
#include <idle.h>
#include <queue_r.h>
#include <thread.h>

/**
 * Size of the kthread collector queue.
 * This doesn't need to be huge because kthreads will rarely exit and even if
 * there would be congestion it doesn't matter because the threads can wait
 * until there is a window for dying.
 */
#define KTHREAD_CQ_LEN 10

static pthread_t kthread_collect_queue_data[KTHREAD_CQ_LEN];
static queue_cb_t kthread_collect_queue =
    QUEUE_INITIALIZER(kthread_collect_queue_data, sizeof(pthread_t),
                      KTHREAD_CQ_LEN * sizeof(pthread_t));

pthread_t kthread_create(struct sched_param * param, size_t stack_size,
                         void * (*kthread_start)(void *), void * arg)
{
    pthread_t tid;

    if (stack_size == 0)
        stack_size = MMU_PGSIZE_COARSE;

    struct buf * bp_stack = geteblk(stack_size);
    if (!bp_stack) {
        KERROR(KERROR_ERR, "Unable to allocate a stack for a new kthread\n");
        return -ENOMEM;
    }

    struct _sched_pthread_create_args tdef = {
        .param      = *param,
        .stack_addr = (void *)bp_stack->b_data,
        .stack_size = bp_stack->b_bcount,
        .flags      = PTHREAD_CREATE_DETACHED,
        .start      = kthread_start,
        .arg1       = (uintptr_t)arg,
        .del_thread = kthread_die,
    };

    tid = thread_create(&tdef, THREAD_MODE_PRIV);
    if (tid < 0) {
        KERROR(KERROR_ERR, "Failed to create a kthread\n");
    }

    return tid;
}

void kthread_die(void * retval)
{
    current_thread->retval = (intptr_t)retval;

    /*
     * Try pushing until it there is space in the queue.
     */
    while (queue_push(&kthread_collect_queue, &current_thread->id) == 0) {
        thread_sleep(100); /* Retry in x ms */
    }

    thread_wait(); /* Wait until collected. */
}

/**
 * kthreads can't kill themselves because that would leave the kernel in
 * undefined state, therefore we use a collector idle task that can kill
 * kthreads that are willing to die.
 */
static void collect_kthreads(uintptr_t arg)
{
    pthread_t tid;

    while (queue_pop(&kthread_collect_queue, &tid)) {
        (void)thread_terminate(tid);
#ifdef configSCHED_DEBUG
        KERROR(KERROR_DEBUG, "Collected kthread %d\n", tid);
#endif
    }
}
IDLE_TASK(collect_kthreads, 0);
