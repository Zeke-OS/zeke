/**
 *******************************************************************************
 * @file    klocks.h
 * @author  Olli Vanhoja
 *
 * @brief   -
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#ifdef configMP
#include <hal/core.h>
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

#define MTX_OPT(mtx, typ) (((mtx)->mtx_flags & (typ)) != 0)

/*
 * Mutex types
 * ===========
 *
 * Lock type            Supported options
 * -----------------------------------------------------------------------------
 * MTX_TYPE_UNDEF       -
 * MTX_TYPE_SPIN        MTX_OPT_SLEEP, MTX_OPT_PRICEIL, MTX_OPT_DINT
 * MTX_TYPE_TICKET      MTX_OPT_PRICEIL, MTX_OPT_DINT
 */

/**
 * Lock type.
 */
enum mtx_type {
    MTX_TYPE_UNDEF = 0,     /*!< mtx un-initialized. */
    MTX_TYPE_SPIN,          /*!< Spin lock. */
    MTX_TYPE_TICKET,        /*!< Use ticket spin locking. */
};


#define MTX_OPT_DEFAULT 0x00 /*!< Default options, none */
#define MTX_OPT_SLEEP   0x10 /*!< Allow timeouted waiting.
                              * Can't be used in interrupt handlers.
                              */
#define MTX_OPT_PRICEIL 0x20 /*!< Use priority ceiling.
                              * Can be only used if lock is always used in
                              * a thread kernel mode and never in
                              * a interrupt handler or during
                              * initialization.
                              */
#define MTX_OPT_DINT    0x40 /*!< Interrupt handler friendly locking.
                              * If this option is used the locking code will
                              * handle cases where a lock can be used in thread
                              * kernel mode as well as in interrupt handler,
                              * otherwise deadlocks may occur if locking is not
                              * carefully planned.
                              * This option will also work on MP.
                              * @note When locked, all interrupts are
                              * disabled.
                              * TODO Not yet MP safe
                              */

/**
 * Sleep/spin mutex.
 */
typedef struct mtx {
    enum mtx_type mtx_type;     /*!< Lock type. */
    unsigned int mtx_flags;     /*!< Option flags. */
    atomic_t mtx_lock;          /*!< Lock value for regular (spin)lock. */
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

/**
 * Initialize a kernel mutex.
 * @param mtx is a pointer to a mutex struct.
 */
void mtx_init(mtx_t * mtx, enum mtx_type type, unsigned int opt);

#define MTX_INITIALIZER(type, opt) (mtx_t){ \
    .mtx_type = (type),                     \
    .mtx_flags = (opt),                     \
    .mtx_lock = ATOMIC_INIT(0),             \
    .ticket.queue = ATOMIC_INIT(0),         \
    .ticket.dequeue = ATOMIC_INIT(0),       \
}

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
 * RW Lock descriptor.
 */
typedef struct rwlock {
    int state; /*!< Lock state. 0 = no lock, -1 = wrlock and 0 < rdlock. */
    int wr_waiting; /*!< writers waiting. */
    struct mtx lock; /*!< Mutex protecting attributes. */
} rwlock_t;

/* Rwlock functions */

/**
 * Initialize an rwlock object.
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
 * Async wait for the write turn on an rwlock.
 * Mark the rwlock as waiting for write lock.
 * @note Only one writer can use this function properly.
 */
void rwlock_wrwait(rwlock_t * l);

/**
 * Remove the waiting status from an rwlock.
 * @note Only one writer can use this function properly.
 */
void rwlock_wrunwait(rwlock_t * l);

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

/**
 * @}
 */

/**
 * @addtogroup cpulock cpulocks
 * Per CPU serialization lock.
 * @sa mtx
 * @{
 */

/**
 * cpulock descriptor.
 */
typedef struct cpulock {
    struct mtx mtx[0];
} cpulock_t;

cpulock_t * cpulock_create(void);
void cpulock_destroy(cpulock_t * lock);
int cpulock_lock(cpulock_t * lock);
void cpulock_unlock(cpulock_t * lock);

/**
 * @}
 */

/**
 * @addtogroup isema
 * Index semaphores.
 * @sa mtx
 * @{
 */

/**
 * Index semaphore descriptor.
 */
typedef atomic_t isema_t;

/**
 * Initialize an index semaphore.
 * @param isema is a pointer to a isema_t array.
 * @param isema_n num_elem() of isema.
 */
void isema_init(isema_t * isema, size_t isema_n);

/**
 * Acquire index from isema array.
 * @param isema is a pointer to a isema_t array.
 * @param isema_n num_elem() of isema.
 */
size_t isema_acquire(isema_t * isema, size_t isema_n);

/**
 * Release an index returned by isema_acquire().
 * @param isema is a pointer to a isema_t array.
 * @param index is an index returned by isema_acquire().
 */
static inline void isema_release(isema_t * isema, size_t index)
{
#ifdef configMP
    cpu_sev(); /* Wakeup cores possibly waiting for an index. */
#endif
    atomic_set(&isema[index], 0);
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* !_KLOCKS_H_ */
