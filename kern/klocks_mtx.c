/**
 *******************************************************************************
 * @file    klocks.h
 * @author  Olli Vanhoja
 * @brief   Kernel space locks.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kstring.h>
#include <kerror.h>
#include <timers.h>
#include <hal/core.h>
#include <thread.h>
#include <klocks.h>

#ifdef configLOCK_DEBUG
#define MTX_TYPE_NOTSUP() do {                                       \
    KERROR(KERROR_ERR, "In %s%s not supported for lock type (%u)\n", \
            whr, __func__, mtx->mtx_type);                           \
} while (0)
#else
#define MTX_TYPE_NOTSUP()
#endif

/**
 * istate for MTX_OPT_DINT.
 * TODO Per CPU istate.
 */
istate_t cpu_istate;

static void priceil_set(mtx_t * mtx)
{
    if (MTX_OPT(mtx, MTX_OPT_PRICEIL)) {
        mtx->pri.p_saved = thread_get_priority(current_thread->id);
        thread_set_priority(current_thread->id, mtx->pri.p_lock);
    }
}

static void priceil_restore(mtx_t * mtx)
{
    if (MTX_OPT(mtx, MTX_OPT_PRICEIL)) {
        /*
         * RFE There is a very rare race condition if some other thread tries to
         * set a new priority for this thread just after this if clause.
         */
        if (thread_get_priority(current_thread->id) == mtx->pri.p_lock)
            thread_set_priority(current_thread->id, mtx->pri.p_saved);
    }
}

void mtx_init(mtx_t * mtx, enum mtx_type type, unsigned int opt)
{
    mtx->mtx_type = type;
    mtx->mtx_flags = opt;
    mtx->mtx_lock = ATOMIC_INIT(0);
    mtx->ticket.queue = ATOMIC_INIT(0);
    mtx->ticket.dequeue = ATOMIC_INIT(0);
#ifdef configLOCK_DEBUG
    mtx->mtx_ldebug = NULL;
#endif
}

#ifndef configLOCK_DEBUG
int mtx_lock(mtx_t * mtx)
#else
int _mtx_lock(mtx_t * mtx, char * whr)
#endif
{
    int ticket;
    const int sleep_mode = MTX_OPT(mtx, MTX_OPT_SLEEP);
#ifdef configLOCK_DEBUG
    unsigned deadlock_cnt = 0;
#endif

    if (mtx->mtx_type == MTX_TYPE_TICKET) {
        ticket = atomic_inc(&mtx->ticket.queue);
    }

    if (MTX_OPT(mtx, MTX_OPT_DINT)) {
        cpu_istate = get_interrupt_state();
        disable_interrupt();
    }

    while (1) {
#ifdef configLOCK_DEBUG
        /*
         * TODO Deadlock detection threshold should depend on lock type and
         *      current priorities.
         */
        if (++deadlock_cnt >= configSCHED_HZ * (configKLOCK_DLTHRES + 1)) {
            char * lwhr = (mtx->mtx_ldebug) ? mtx->mtx_ldebug : "?";

            KERROR(KERROR_DEBUG,
                   "Deadlock detected:\n%s WAITING\n%s LOCKED\n",
                   whr, lwhr);

            deadlock_cnt = 0;
        }
#endif

        if (sleep_mode && (current_thread->wait_tim == -2))
            return -EWOULDBLOCK;

        switch (mtx->mtx_type) {
        case MTX_TYPE_SPIN:
            if (!atomic_test_and_set(&mtx->mtx_lock))
                goto out;
            break;

        case MTX_TYPE_TICKET:
            if (atomic_read(&mtx->ticket.dequeue) == ticket) {
                atomic_set(&mtx->mtx_lock, 1);
                goto out;
            }

            thread_yield(THREAD_YIELD_LAZY);
            break;

        default:
            MTX_TYPE_NOTSUP();
            if (MTX_OPT(mtx, MTX_OPT_DINT))
                set_interrupt_state(cpu_istate);

            return -ENOTSUP;
        }

#ifdef configMP
        cpu_wfe(); /* Sleep until event. */
#endif
    }
out:

    /* Handle priority ceiling. */
    priceil_set(mtx);

#ifdef configLOCK_DEBUG
    mtx->mtx_ldebug = whr;
#endif

    return 0;
}

static void mtx_wakeup(void * arg)
{
    timers_release(current_thread->wait_tim);
    current_thread->wait_tim = -2; /* Magic */
}

#ifndef configLOCK_DEBUG
int mtx_sleep(mtx_t * mtx, long timeout)
#else
int _mtx_sleep(mtx_t * mtx, long timeout, char * whr)
#endif
{
    int retval;

    if (MTX_OPT(mtx, MTX_OPT_DINT))
        goto fail;

    if (timeout > 0) {
        KASSERT(current_thread->wait_tim < 0,
                "Can't have multiple wait timers per thread");
        current_thread->wait_tim =
            timers_add(mtx_wakeup, mtx, TIMERS_FLAG_ONESHOT, timeout);
        if (current_thread->wait_tim < 0)
            return -EWOULDBLOCK;

        retval = mtx_lock(mtx);
        timers_release(current_thread->wait_tim);
        current_thread->wait_tim = -1;
    } else if (mtx->mtx_type == MTX_TYPE_SPIN) {
        retval = mtx_lock(mtx);
    } else {
fail:
        MTX_TYPE_NOTSUP();
        return -ENOTSUP;
    }

    return retval;
}

#ifndef configLOCK_DEBUG
int mtx_trylock(mtx_t * mtx)
#else
int _mtx_trylock(mtx_t * mtx, char * whr)
#endif
{
    int ticket;
    int retval;

    if (MTX_OPT(mtx, MTX_OPT_DINT)) {
        cpu_istate = get_interrupt_state();
        disable_interrupt();
    }

    switch (mtx->mtx_type) {
    case MTX_TYPE_SPIN:
        retval = atomic_test_and_set(&mtx->mtx_lock);
        break;

    case MTX_TYPE_TICKET:
        ticket = atomic_inc(&mtx->ticket.queue);

        if (atomic_read(&mtx->ticket.dequeue) == ticket) {
            atomic_set(&mtx->mtx_lock, 1);
            return 0; /* Got it */
        } else {
            atomic_dec(&mtx->ticket.queue);
             if (MTX_OPT(mtx, MTX_OPT_DINT))
                set_interrupt_state(cpu_istate);
            return 1; /* No luck */
        }
        break;

    default:
        MTX_TYPE_NOTSUP();
        if (MTX_OPT(mtx, MTX_OPT_DINT))
            set_interrupt_state(cpu_istate);

        return -ENOTSUP;
    }

    /* Handle priority ceiling. */
    priceil_set(mtx);

#ifdef configLOCK_DEBUG
    mtx->mtx_ldebug = whr;
#endif

    return retval;
}

void mtx_unlock(mtx_t * mtx)
{
    if (MTX_OPT(mtx, MTX_OPT_SLEEP) && (current_thread->wait_tim >= 0)) {
        timers_release(current_thread->wait_tim);
        current_thread->wait_tim = -1;
    }

#ifdef configLOCK_DEBUG
    mtx->mtx_ldebug = NULL;
#endif

    if (mtx->mtx_type == MTX_TYPE_TICKET)
        atomic_inc(&mtx->ticket.dequeue);
    atomic_set(&mtx->mtx_lock, 0);

    if (MTX_OPT(mtx, MTX_OPT_DINT))
        set_interrupt_state(cpu_istate);

    /* Restore priority ceiling. */
    priceil_restore(mtx);

#ifdef configMP
    cpu_sev(); /* Wakeup cores possibly waiting for lock. */
#endif
}

int mtx_test(mtx_t * mtx)
{
    return atomic_read(&mtx->mtx_lock);
}
