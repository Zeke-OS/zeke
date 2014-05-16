/**
 *******************************************************************************
 * @file    klocks.h
 * @author  Olli Vanhoja
 *
 * @brief   -
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef KLOCKS_H_
#define KLOCKS_H_

//#define LOCK_DEBUG 1

#ifdef LOCK_DEBUG
#include <kerror.h>
#endif

/* Mutex types */
#define MTX_DEF     0x00 /*!< DEFAULT (sleep) lock */
#define MTX_SPIN    0x01 /*!< Spin lock. */

/*
 * Sleep/spin mutex.
 *
 * All mutex implementations must always have a member called mtx_lock.
 * Other locking primitive structures are not allowed to use this name
 * for their members.
 * If this rule needs to change, the bits in the mutex implementation must
 * be modified appropriately.
 */
typedef struct mtx {
    intptr_t _alignment_bytes;
    unsigned int mtx_tflags;    /*!< Type flags. */
    volatile int mtx_lock;      /*!< Lock value. */
#ifdef LOCK_DEBUG
    char * mtx_ldebug;
#endif
} mtx_t;

/**
 * RW Lock.
 */
typedef struct rwlock {
    int state; /*!< Lock state. 0 = no lock, -1 = wrlock and 0 < rdlock. */
    int wr_waiting; /*!< writers waiting. */
    struct mtx lock; /*!< Mutex protecting attributes. */
} rwlock_t;

/* Mutex functions */
#ifndef LOCK_DEBUG
int mtx_spinlock(mtx_t * mtx);
int mtx_trylock(mtx_t * mtx);
#else /* Debug versions */
#define mtx_spinlock(mtx)   _mtx_spinlock(mtx, _KERROR_WHERESTR)
#define mtx_trylock(mtx)    _mtx_trylock(mtx, _KERROR_WHERESTR)
int _mtx_spinlock(mtx_t * mtx, char * whr);
int _mtx_trylock(mtx_t * mtx, char * whr);
#endif

void mtx_init(mtx_t * mtx, unsigned int type);
void mtx_unlock(mtx_t * mtx);

/* Rwlock functions */
void rwlock_init(rwlock_t * l);
void rwlock_wrlock(rwlock_t * l);
int rwlock_trywrlock(rwlock_t * l);
void rwlock_wrunlock(rwlock_t * l);
void rwlock_rdlock(rwlock_t * l);
int rwlock_tryrdlock(rwlock_t * l);
void rwlock_rdunlock(rwlock_t * l);

#endif /* !_KLOCKS_H_ */
