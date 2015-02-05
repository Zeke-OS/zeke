/**
 *******************************************************************************
 * @file    klocks.h
 * @author  Olli Vanhoja
 *
 * @brief   -
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @addtogroup klocks
 * @{
 */

#pragma once
#ifndef KLOCKS_H_
#define KLOCKS_H_

#include <sys/types_pthread.h>
#include <machine/atomic.h>
#ifdef configLOCK_DEBUG
#include <kerror.h>
#endif

/**
 * @addtogroup mtx mtx_init, mtx_lock, mtx_trylock
 * Kernel mutex lock functions.
 * This implementation of kernel locks is intended for handling mutex on
 * multithreaded/pre-emptive kernel.
 *
 * mtx_lock() spins until lock is achieved and returns 0 if lock achieved;
 * 3 if not allowed; Otherwise value other than zero.
 *
 * mtx_trylock() tries to achieve lock and returns 0 if lock is achieved
 * but doesn't ever block or spin.
 * @sa rwlock
 * @{
 */

#define MTX_TYPE(mtx, typ) (((mtx)->mtx_tflags & (typ)) != 0)

/* Mutex types */
#define MTX_TYPE_UNDEF      0x00 /*!< mtx un-initialized. */
#define MTX_TYPE_SPIN       0x01 /*!< Spin lock. */
#define MTX_TYPE_TICKET     0x02 /*!< Use ticket spin locking. */
#define MTX_TYPE_SLEEP      0x10 /*!< Allow timeouted waiting. */
#define MTX_TYPE_PRICEIL    0x20 /*!< Use priority ceiling. */

/**
 * Sleep/spin mutex.
 *
 * All mutex implementations must always have a member called mtx_lock.
 * Other locking primitive structures are not allowed to use this name
 * for their members.
 * If this rule needs to change, the bits in the mutex implementation must
 * be modified appropriately.
 */
typedef struct mtx {
    intptr_t _alignment;
    unsigned int mtx_tflags;    /*!< Type flags. */
    volatile int mtx_lock;      /*!< Lock value for regular (spin)lock. */
    struct ticket {
        atomic_t queue;
        atomic_t dequeue;
    } ticket;                   /*!< Ticket lock. */
    struct pri {
        int p_lock;
        int p_saved;
    } pri;
#ifdef configLOCK_DEBUG
    char * mtx_ldebug;
#endif
} mtx_t;

/* Mutex functions */
#ifndef configLOCK_DEBUG
int mtx_lock(mtx_t * mtx);
int mtx_sleep(mtx_t * mtx, long timeout);
int mtx_trylock(mtx_t * mtx);
#else /* Debug versions */
#define mtx_lock(mtx)   _mtx_lock(mtx, _KERROR_WHERESTR)
#define mtx_sleep(mtx, timeout) _mtx_sleep(mtx, timeout, _KERROR_WHERESTR)
#define mtx_trylock(mtx)    _mtx_trylock(mtx, _KERROR_WHERESTR)
int _mtx_lock(mtx_t * mtx, char * whr);
int _mtx_sleep(mtx_t * mtx, long timeout, char * whr);
int _mtx_trylock(mtx_t * mtx, char * whr);
#endif

/**
 * Initialize a kernel mutex.
 * @param mtx is a mutex struct.
 * @param type is the type of the mutex.
 */
void mtx_init(mtx_t * mtx, unsigned int type);

/**
 * Release kernel mtx lock.
 * @param mtx is a mutex struct.
 */
void mtx_unlock(mtx_t * mtx);

/**
 * Test if locked.
 * @param mtx is a mutex struct.
 */
int mtx_test(mtx_t * mtx);

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
 * RW Lock.
 */
typedef struct rwlock {
    int state; /*!< Lock state. 0 = no lock, -1 = wrlock and 0 < rdlock. */
    int wr_waiting; /*!< writers waiting. */
    struct mtx lock; /*!< Mutex protecting attributes. */
} rwlock_t;

/* Rwlock functions */

/**
 * Initialize rwlock object.
 * @param l is the rwlock.
 */
void rwlock_init(rwlock_t * l);

/**
 * Get write lock to rwlock.
 * @param l is the rwlock.
 */
void rwlock_wrlock(rwlock_t * l);

/**
 * Try to get write lock.
 * @param l is the rwlock.
 * @return Returns 0 if lock achieved; Otherwise value other than zero.
 */
int rwlock_trywrlock(rwlock_t * l);

/**
 * Release write lock.
 * @param l is the rwlock.
 */
void rwlock_wrunlock(rwlock_t * l);

/**
 * Get reader's lock.
 * @param l is the rwlock.
 */
void rwlock_rdlock(rwlock_t * l);

/**
 * Try to get reader's lock.
 * @param is the rwlock.
 * @return Returns 0 if lock achieved; Otherwise value other than zero.
 */
int rwlock_tryrdlock(rwlock_t * l);

/**
 * Release reader's lock.
 * @param l is the rwlock.
 */
void rwlock_rdunlock(rwlock_t * l);

#endif /* !_KLOCKS_H_ */

/**
 * @}
 */

/**
 * @}
 */
