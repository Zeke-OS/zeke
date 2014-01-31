/**
 *******************************************************************************
 * @file    klocks.h
 * @author  Olli Vanhoja
 * @brief   Kernel space locks
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

/*
 * This implementation of kernel locks is intended for handling mutex on
 * multithreaded/pre-emptive kernel.
 */

#define KERNEL_INTERNAL
#include <hal/hal_core.h>
#include <sys/_mutex.h>

/**
 * Initialize a kernel mutex.
 * @param mtx is a mutex struct.
 */
void mtx_init(mtx_t * mtx)
{
    mtx->mtx_owner = 0;
    mtx->mtx_lock = 0;
#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = 0;
#endif
}

/**
 * Get kernel mtx lock.
 * mtx lock spins until lock is achieved.
 * @param mtx is a mutex struct.
 * @return Returns 0 if lock achieved.
 */
#ifndef LOCK_DEBUG
int mtx_spinlock(mtx_t * mtx)
#else
int _mtx_spinlock(mtx_t * mtx, char * whr)
#endif
{
    while(test_and_set(&(mtx->mtx_lock)));
#ifdef LOCK_DEBUG
    mtx->mtx_ldebug = whr;
#endif

    return 0;
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

    retval = test_and_set(&(mtx->mtx_lock));
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
}
