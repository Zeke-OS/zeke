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

/** @addtogroup Kernel
 * @{
 */

/*
 * This implementation of kernel locks is intended for handling mutex on
 * multithreaded/pre-emptive kernel.
 */

#define KERNEL_INTERNAL
#include <hal/hal_core.h>
#include <klocks.h>

/**
 * @addtogroup mtx mtx_init, mtx_spinlock, mtx_trylock
 * Kernel mutex lock functions.
 * @sa rwlock
 * @{
 */

/**
 * Initialize a kernel mutex.
 * @param mtx is a mutex struct.
 * @param type is the type of the mutex.
 */
void mtx_init(mtx_t * mtx, unsigned int type)
{
    mtx->mtx_lock = 0;
    mtx->mtx_tflags = type;
#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = 0;
#endif
}

/**
 * Get kernel mtx lock.
 * mtx lock spins until lock is achieved.
 * @param mtx is a mutex struct.
 * @return  Returns 0 if lock achieved; 3 if not allowed; Otherwise value other
 *          than zero.
 */
#ifndef LOCK_DEBUG
int mtx_spinlock(mtx_t * mtx)
#else
int _mtx_spinlock(mtx_t * mtx, char * whr)
#endif
{
    int retval = 0;

    if ((mtx->mtx_tflags & MTX_SPIN) == 0) {
        retval = 3; /* Not allowed */
        goto out;
    }

    while(test_and_set((int *)(&(mtx->mtx_lock)))) {
#if configMP != 0
        cpu_wfe(); /* Sleep until event. */
#endif
    }

#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = whr;
#endif

out:
    return retval;
}

/**
 * Try to get kernel mtx lock.
 * @param mtx is a mutex struct.
 * @return Returns 0 if lock achieved.
 */
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

/**
 * Release kernel mtx lock.
 * @param mtx is a mutex struct.
 */
void mtx_unlock(mtx_t * mtx)
{
    mtx->mtx_lock = 0;
#if configMP != 0
    cpu_sev(); /* Wakeup cores possible waiting for lock. */
#endif
}

/**
 * Test if locked.
 * @param mtx is a mutex struct.
 */
int mtx_test(mtx_t * mtx)
{
    return test_lock((int *)(&(mtx->mtx_lock)));
}

/**
 * @}
 */

/**
 * @addtogroup rwlock rwlocks
 * Readers-writer lock implementation for in-kernel usage.
 * @sa mtx
 * @{
 */

/**
 * Initialize rwlock object.
 * @param l is the rwlock.
 */
void rwlock_init(rwlock_t * l)
{
    l->state = 0;
    l->wr_waiting = 0;
    mtx_init(&(l->lock), MTX_DEF | MTX_SPIN);
}

/**
 * Get write lock to rwlock.
 * @param l is the rwlock.
 */
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

/**
 * Try to get write lock.
 * @param l is the rwlock.
 * @return Returns 0 if lock achieved; Otherwise value other than zero.
 */
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

/**
 * Release write lock.
 * @param l is the rwlock.
 */
void rwlock_wrunlock(rwlock_t * l)
{
    mtx_spinlock(&(l->lock));
    if (l->state == -1) {
        l->state = 0;
    }
    mtx_unlock(&(l->lock));
}

/**
 * Get reader's lock.
 * @param l is the rwlock.
 */
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

/**
 * Try to get reader's lock.
 * @param is the rwlock.
 * @return Returns 0 if lock achieved; Otherwise value other than zero.
 */
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

/**
 * Release reader's lock.
 * @param l is the rwlock.
 */
void rwlock_rdunlock(rwlock_t * l)
{
    mtx_spinlock(&(l->lock));
    if (l->state > 0) {
        l->state--;
    }
    mtx_unlock(&(l->lock));
}

/**
 * @}
 */

/**
 * @}
 */
