/**
 *******************************************************************************
 * @file    kernel.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 * @section LICENSE
 * Copyright (c) 2013 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/** @addtogroup Library_Functions
  * @{
  */

#include <hal/hal_core.h>
#include <syscalldef.h>
#include <syscall.h>
#include <pthread.h>

/** @addtogroup Threads
  * @{
  */

int pthread_create(pthread_t * thread, const pthread_attr_t * attr,
                void * (*start_routine)(void *), void * arg)
{
    ds_pthread_create_t args = {
        .thread     = thread,
        .start      = start_routine,
        .def        = attr,
        .argument   = arg
    };
    int result;

    result = (int)syscall(SYSCALL_SCHED_THREAD_CREATE, &args);

    /* Request immediate context switch */
    req_context_switch();

    return result;
}

pthread_t pthread_self(void)
{
    return (pthread_t)syscall(SYSCALL_SCHED_THREAD_GETTID, NULL);
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup Mutex
  * @{
  */

/* Mutex Management ***********************************************************
 * NOTE: osMutexId/mutex_id is a direct pointer to a os_mutex_cb structure.
 * POSIX compliant functions for mutex are: 
 * pthread_mutex_init(mutex,attr)
 * pthread_mutex_destroy(mutex)
 * 
 * pthread_mutexattr_init(attr)
 * pthread_mutexattr_destroy(attr)
 * 
 * pthread_mutex_lock(mutex) - Thread will become blocked
 * pthread_mutex_unlock(mutex)
 * pthread_mutex_trylock(mutex) - Thread will not be blocked
 */
/** TODO Should these functions be renamed?*/

/** TODO Add headers to pthreads.h! */

mutex_cb_t pthread_mutex_init(mutex_cb_t * mutex)
{
    /*TODO What about the parameter osMutexDef_t?*/
    mutex_cb_t cb = {
        .mutex     = mutex
        .thread_id = pthread_self,
        .lock      = 0,
        .strategy  = mutex_def->strategy
    };

    return cb;
}

int pthread_mutex_lock(mutex_cb_t * mutex) 
{
    /*If not succesful, we switch context. Should be posix compliant.*/
    while((int)syscall(SYSCALL_MUTEX_TEST_AND_SET, (void*)(&(mutex->lock))) !=0)
    {
        req_context_switch();
    }
    return 0;
    
}

int pthread_mutex_trylock(mutex_cb_t * mutex)
{
    int result;

    result = (int)syscall(SYSCALL_MUTEX_TEST_AND_SET, (void*)(&(mutex->lock)));
    if (result == 0) 
    {
        mutex->thread_id = pthread_self();
        return 0;
    }
    /*Should we request context switch if not succesful?*/
    return 1;
}

int pthread_mutex_unlock(mutex_cb_t * mutex)
{
    if (mutex->thread_id == pthread_self()) 
    {
        mutex->lock = 0;
        return 0;
    }
    return 1;
}

/**
  * @}
  */