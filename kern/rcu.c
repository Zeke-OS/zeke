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

    ctx = rcu_read_lock(); /* Use read lock to prevent a clock tick. */
    do {
        cbd->callback = fn;
        cbd->callback_arg = cbd;
        cbd->next = rcu_reclaim_list[ctx.selector];
    } while (atomic_cmpxchg_ptr((void **)(&rcu_reclaim_list[ctx.selector]),
                                cbd->next, cbd) != cbd->next);
    rcu_read_unlock(&ctx);
}

void rcu_synchronize(void)
{
    int old, old_clock, new;
    struct rcu_lock_ctx ctx;

    do {
        int tick;

retry:
        old = atomic_read(&rcu_ctrl);
        old_clock = RCU_CTRL_TO_CLOCK(old);
        tick = (RCU_GET_CTR(old, old_clock) != 0);
        if (!tick) {
            /*
             * We yield until we get a tick. A tick means that all readers on
             * the current grace period are ready.
             */
            if (current_thread->id == rcu_sync_thread_tid) {
                thread_sleep(1000);
            } else {
                thread_yield(THREAD_YIELD_IMMEDIATE);
            }
            goto retry;
        }
        new = old ^ (1 << RCU_CLOCK_OFFSET);
    } while (atomic_cmpxchg(&rcu_ctrl, old, new) != old);

    ctx = rcu_read_lock(); /* Prevent a new clock tick while we are here. */
    /*
     * If we miss a window for callbacks now we should always get back here
     * later on anyway, so it's not critical.
     */
    if (ctx.selector != old_clock && rcu_reclaim_list[old_clock]) {
        struct rcu_cb * cbd = rcu_reclaim_list[old_clock];
        struct rcu_cb * next;
        do {
            next = cbd->next;
            cbd->callback(cbd->callback_arg);
        } while ((cbd = next));
    }
    rcu_read_unlock(&ctx);
}

static void * rcu_sync_thread(void * arg)
{
    while (1) {
        rcu_synchronize();
    }
}

int __kinit__ rcu_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("rcu");

    struct buf * bp_stack = geteblk(MMU_PGSIZE_COARSE);
    if (!bp_stack) {
        KERROR(KERROR_ERR, "Can't allocate a stack for rcu sync thread.");
        return -ENOMEM;
    }

    struct _sched_pthread_create_args tdef = {
        .param.sched_policy = SCHED_OTHER,
        .param.sched_priority = NICE_MAX,
        .stack_addr = (void *)bp_stack->b_data,
        .stack_size = bp_stack->b_bcount,
        .flags      = 0,
        .start      = rcu_sync_thread,
        .arg1       = 0,
    };

    rcu_sync_thread_tid = thread_create(&tdef, THREAD_MODE_PRIV);
    if (rcu_sync_thread_tid < 0) {
        KERROR(KERROR_ERR, "Failed to create a thread for rcu sync");
        return rcu_sync_thread_tid;
    }

    return 0;
}
