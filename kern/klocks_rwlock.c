/**
 *******************************************************************************
 * @file    klocks_rwlock.h
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
#include <thread.h>
#include <klocks.h>

void rwlock_init(rwlock_t * l)
{
    l->state = 0;
    l->wr_waiting = 0;
    mtx_init(&l->lock, MTX_TYPE_SPIN, 0);
}

void rwlock_wrlock(rwlock_t * l)
{
    mtx_lock(&(l->lock));
    if (l->state == 0) {
        goto get_wrlock;
    } else {
        l->wr_waiting++;
    }
    mtx_unlock(&l->lock);

    /* Try to minimize locked time. */
    while (1) {
        if (l->state == 0) {
            mtx_lock(&l->lock);
            if (l->state == 0) {
                l->wr_waiting--;
                goto get_wrlock;
            }
            mtx_unlock(&l->lock);
        }
    }

get_wrlock:
    l->state = -1;
    mtx_unlock(&l->lock);
}

int rwlock_trywrlock(rwlock_t * l)
{
    int retval = 1;

    if (mtx_trylock(&l->lock))
        goto out;

    if (l->state == 0) {
        l->state = -1;
        retval = 0;
    }
    mtx_unlock(&l->lock);

out:
    return retval;
}

void rwlock_wrunlock(rwlock_t * l)
{
    mtx_lock(&(l->lock));
    if (l->state == -1) {
        l->state = 0;
    }
    mtx_unlock(&l->lock);
}

void rwlock_rdlock(rwlock_t * l)
{
    mtx_lock(&(l->lock));
    /* Don't take lock if any writer is waiting. */
    if (l->wr_waiting == 0 && l->state >= 0) {
        goto get_rdlock;
    }
    mtx_unlock(&(l->lock));

    /* Try to minimize locked time. */
    while (1) {
        if (l->wr_waiting == 0 && l->state >= 0) {
            mtx_lock(&(l->lock));
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
    mtx_unlock(&l->lock);

out:
    return retval;
}

void rwlock_rdunlock(rwlock_t * l)
{
    mtx_lock(&l->lock);
    if (l->state > 0) {
        l->state--;
    }
    mtx_unlock(&l->lock);
}
