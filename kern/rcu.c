/**
 *******************************************************************************
 * @file    rcu.c
 * @author  Olli Vanhoja
 * @brief   Realtime friendly Read-Copy-Update.
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

#include <machine/atomic.h>
#include <buf.h>
#include <hal/core.h>
#include <idle.h>
#include <kerror.h>
#include <kinit.h>
#include <thread.h>
#include <rcu.h>

static atomic_t rcu_ctrl = ATOMIC_INIT(0);

#define RCU_CTR_A_OFFSET     0
#define RCU_CTR_B_OFFSET    15
#define RCU_CLOCK_OFFSET    30

#define RCU_CTR_MASK        0x7fff
#define RCU_CLOCK_MASK      0x1

#define RCU_CTRL_TO_CTR_A(_x_) (((_x_) >> RCU_CTR_A_OFFSET) & RCU_CTR_MASK)
#define RCU_CTRL_TO_CTR_B(_x_) (((_x_) >> RCU_CTR_B_OFFSET) & RCU_CTR_MASK)
#define RCU_CTRL_TO_CLOCK(_x_) (((_x_) >> RCU_CLOCK_OFFSET) & RCU_CLOCK_MASK);

#define RCU_CTR_ONE(_clock_) \
    ((_clock_) ? 1 << RCU_CTR_B_OFFSET : 1 << RCU_CTR_A_OFFSET)

#define RCU_GET_CTR(_ctrl_, _clock_) \
    ((_clock_) ? RCU_CTRL_TO_CTR_B(_ctrl_) : RCU_CTRL_TO_CTR_A(_ctrl_))

#define RCU_GET_CURCTR(_ctrl_) \
    (RCU_GET_CTR((_ctrl_), RCU_CTRL_TO_CLOCK(_ctrl_)))

static struct rcu_cb * rcu_reclaim_list[2];

static pthread_t rcu_sync_thread_tid;

struct rcu_lock_ctx rcu_read_lock(void)
{
    int old, new, selector;

    do {
        old = atomic_read(&rcu_ctrl);
        selector = RCU_CTRL_TO_CLOCK(old);
        new = old + RCU_CTR_ONE(selector);
    } while (atomic_cmpxchg(&rcu_ctrl, old, new) != old);

    return (struct rcu_lock_ctx){ .selector = selector };
}

void rcu_read_unlock(struct rcu_lock_ctx * restrict ctx)
{
    int old, new;

    do {
        old = atomic_read(&rcu_ctrl);
        new = old - RCU_CTR_ONE(ctx->selector);
    } while (atomic_cmpxchg(&rcu_ctrl, old, new) != old);
}

void rcu_call(struct rcu_cb * cbd, void (*fn)(struct rcu_cb *))
{
    struct rcu_lock_ctx ctx;
    struct rcu_cb * old;
    struct rcu_cb ** list_head;

    ctx = rcu_read_lock(); /* Use read lock to prevent a clock tick. */
    list_head = &rcu_reclaim_list[ctx.selector];
    do {
        cbd->callback = fn;
        cbd->callback_arg = cbd;
        old = *list_head;
        cbd->next = old;
    } while (atomic_cmpxchg_ptr((void **)(list_head), old, cbd) != old);
    rcu_read_unlock(&ctx);
}

static inline void rcu_yield(void)
{
#if configRCU_SYNC_HZ > 0
    if (current_thread->id == rcu_sync_thread_tid) {
        thread_sleep(configRCU_SYNC_HZ);
    } else
#endif
    {
        thread_yield(THREAD_YIELD_IMMEDIATE);
    }
}

static void rcu_wait_for_readers(int old_clock)
{
    while (1) {
        int ctrl = atomic_read(&rcu_ctrl);
        if (RCU_GET_CTR(ctrl, old_clock) == 0)
            return;
        rcu_yield();
    }
}

void rcu_synchronize(void)
{
    static mtx_t rcu_sync_lock = MTX_INITIALIZER(MTX_TYPE_TICKET,
                                                 MTX_OPT_DEFAULT);
    int old, old_clock, new;

    /*
     * Callers of this function will get thru this in call order since
     * rcu_sync_lock is a ticket lock.
     */
    mtx_lock(&rcu_sync_lock);

    /* Stage 1: Advance the RCU clock. */
    do {
retry:
        old = atomic_read(&rcu_ctrl);
        old_clock = RCU_CTRL_TO_CLOCK(old);
        const int tick = RCU_GET_CTR(old, old_clock ^ 1) == 0;
        if (!tick) {
            /*
             * We yield until we get a tick. A tick means that all readers of
             * the other counter on the current grace period are ready.
             */
            rcu_yield();
            goto retry;
        }
        new = old ^ (1 << RCU_CLOCK_OFFSET);
    } while (atomic_cmpxchg(&rcu_ctrl, old, new) != old);

    /*
     * Stage 2: Reclaim orphaned resources.
     * Wait until all the readers of the previous counter are ready.
     */
    rcu_wait_for_readers(old_clock);
    if (rcu_reclaim_list[old_clock]) {
        struct rcu_cb * cbd = rcu_reclaim_list[old_clock];
        struct rcu_cb * next;

        /*
         * Call reclaim callbacks for registered CBs.
         */
        do {
            next = cbd->next;
            cbd->callback(cbd->callback_arg);
        } while ((cbd = next));
    }
    rcu_reclaim_list[old_clock] = NULL;

    mtx_unlock(&rcu_sync_lock);
}

#if configRCU_SYNC_HZ > 0
static void * rcu_sync_thread(void * arg)
{
    while (1) {
        rcu_synchronize();
        thread_sleep(configRCU_SYNC_HZ);
    }
}

int __kinit__ rcu_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("rcu sync");

    struct sched_param param = {
        .sched_policy = SCHED_OTHER,
        .sched_priority = NICE_MAX,
    };

    rcu_sync_thread_tid = kthread_create(&param, 0, rcu_sync_thread, NULL);
    if (rcu_sync_thread_tid < 0) {
        KERROR(KERROR_ERR, "Failed to create a thread for rcu sync\n");
        return rcu_sync_thread_tid;
    }

    return 0;
}
#endif
