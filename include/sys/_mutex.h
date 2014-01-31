/**
 *******************************************************************************
 * @file    _mutex.h
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

#ifndef _SYS__MUTEX_H_
#define _SYS__MUTEX_H_
#ifdef KERNEL_INTERNAL

//#define LOCK_DEBUG 1

#ifdef LOCK_DEBUG
#include <kerror.h>
#endif

/* Mutex types */
#define MTX_DEF     0x00000000  /*!< DEFAULT (sleep) lock */
#define MTX_SPIN    0x00000001  /*!< Spin lock */

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
    volatile void * mtx_owner;  /*!< Pointer to optional owner information. */
#ifdef LOCK_DEBUG
    char * mtx_ldebug;
#endif
    unsigned int mtx_tflags;    /*!< Type flags. */
    volatile int mtx_lock;      /*!< Lock value. */
} mtx_t;

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

#endif
#endif /* !_SYS__MUTEX_H_ */
