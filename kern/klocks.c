/**
 *******************************************************************************
 * @file    klocks.h
 * @author  Olli Vanhoja
 * @brief   Kernel space locks.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL
#include <errno.h>
#include <hal/core.h>
#include <klocks.h>

void mtx_init(mtx_t * mtx, unsigned int type)
{
    mtx->mtx_tflags = type;
    mtx->mtx_lock = 0;
    mtx->ticket.queue = ATOMIC_INIT(0);
    mtx->ticket.dequeue = ATOMIC_INIT(0);
#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = 0;
#endif
}

#ifndef LOCK_DEBUG
int mtx_spinlock(mtx_t * mtx)
#else
int _mtx_spinlock(mtx_t * mtx, char * whr)
#endif
{
    if (!MTX_TYPE(mtx, MTX_TYPE_SPIN)) {
#ifdef LOCK_DEBUG
        char buf[80];

        ksprintf(buf, sizeof(buf), "Invalid lock type. Caller: %s", whr);
        KERROR(KERROR_DEBUG, buf);
#endif
        return -ENOTSUP;
    }

    if (MTX_TYPE(mtx, MTX_TYPE_TICKET)) {
        const int ticket = atomic_inc(&(mtx->ticket.queue));

        while (atomic_read(&(mtx->ticket.dequeue)) != ticket) {
                sched_current_thread_yield();
#if configMP != 0
                cpu_wfe(); /* Sleep until event. */
#endif
        }
    } else {
        while (test_and_set((int *)(&(mtx->mtx_lock)))) {
#if configMP != 0
            cpu_wfe(); /* Sleep until event. */
#endif
        }
    }

#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = whr;
#endif

    return 0;
}

#ifndef LOCK_DEBUG
int mtx_trylock(mtx_t * mtx)
#else
int _mtx_trylock(mtx_t * mtx, char * whr)
#endif
{
    int retval;

    retval = test_and_set((int *)(&(mtx->mtx_lock)));
#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = whr;
#endif

    return retval;
}

void mtx_unlock(mtx_t * mtx)
{
    if (MTX_TYPE(mtx, MTX_TYPE_TICKET)) {
        atomic_inc(&(mtx->ticket.dequeue));
    } else {
        mtx->mtx_lock = 0;
    }

#if configMP != 0
    cpu_sev(); /* Wakeup cores possibly waiting for lock. */
#endif
}

int mtx_test(mtx_t * mtx)
{
    return test_lock((int *)(&(mtx->mtx_lock)));
}

void rwlock_init(rwlock_t * l)
{
    l->state = 0;
    l->wr_waiting = 0;
    mtx_init(&(l->lock), MTX_TYPE_SPIN);
}

void rwlock_wrlock(rwlock_t * l)
{
    mtx_spinlock(&(l->lock));
    if (l->state == 0) {
        goto get_wrlock;
    } else {
        l->wr_waiting++;
    }
    mtx_unlock(&(l->lock));

    /* Try to minimize locked time. */
    while (1) {
        if (l->state == 0) {
            mtx_spinlock(&(l->lock));
            if (l->state == 0) {
                l->wr_waiting--;
                goto get_wrlock;
            }
            mtx_unlock(&(l->lock));
        }
    }

get_wrlock:
    l->state = -1;
    mtx_unlock(&(l->lock));
}

int rwlock_trywrlock(rwlock_t * l)
{
    int retval = 1;

    if (mtx_trylock(&(l->lock)))
        goto out;

    if (l->state == 0) {
        l->state = -1;
        retval = 0;
    }
    mtx_unlock(&(l->lock));

out:
    return retval;
}

void rwlock_wrunlock(rwlock_t * l)
{
    mtx_spinlock(&(l->lock));
    if (l->state == -1) {
        l->state = 0;
    }
    mtx_unlock(&(l->lock));
}

void rwlock_rdlock(rwlock_t * l)
{
    mtx_spinlock(&(l->lock));
    /* Don't take lock if any writer is waiting. */
    if (l->wr_waiting == 0 && l->state >= 0) {
        goto get_rdlock;
    }
    mtx_unlock(&(l->lock));

    /* Try to minimize locked time. */
    while (1) {
        if (l->wr_waiting == 0 && l->state >= 0) {
            mtx_spinlock(&(l->lock));
            if (l->wr_waiting == 0 && l->state >= 0) {
                goto get_rdlock;
            }
            mtx_unlock(&(l->lock));
        }
    }

get_rdlock:
    l->state++;
    mtx_unlock(&(l->lock));
}

int rwlock_tryrdlock(rwlock_t * l)
{
    int retval = 1;

    if (mtx_trylock(&(l->lock)))
        goto out;

    if (l->wr_waiting == 0 && l->state >= 0) {
        l->state++;
        retval = 0;
    }
    mtx_unlock(&(l->lock));

out:
    return retval;
}

void rwlock_rdunlock(rwlock_t * l)
{
    mtx_spinlock(&(l->lock));
    if (l->state > 0) {
        l->state--;
    }
    mtx_unlock(&(l->lock));
}
