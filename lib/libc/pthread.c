/**
 *******************************************************************************
 * @file    pthread.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 * @section LICENSE
 * Copyright (c) 2013 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <syscall.h>
#include <pthread.h>

int pthread_create(pthread_t * thread, const pthread_attr_t * attr,
                void * (*start_routine)(void *), void * arg)
{
    struct _ds_pthread_create args = {
        .thread     = thread,
        .start      = start_routine,
        .def        = attr,
        .argument   = arg,
        .del_thread = pthread_exit
    };
    int result;

    result = (int)syscall(SYSCALL_THREAD_CREATE, &args);

    /* Request immediate context switch */
    req_context_switch();

    return result;
}

pthread_t pthread_self(void)
{
    return (pthread_t)syscall(SYSCALL_THREAD_GETTID, NULL);
}

void pthread_exit(void * retval)
{
    (void)syscall(SYSCALL_THREAD_DIE, retval);
    /* Syscall will not return */
}

int pthread_detach(pthread_t thread)
{
    return syscall(SYSCALL_THREAD_DETACH, &thread);
}

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

int pthread_mutex_init(pthread_mutex_t * mutex,
                       const pthread_mutexattr_t * attr)
{
    /*TODO What if attr is not set?*/
    mutex->thread_id    = pthread_self();
    mutex->lock         = 0;
    mutex->strategy     = attr->strategy;

    return 0;
}

int pthread_mutex_lock(pthread_mutex_t * mutex)
{
    /*If not succesful, we switch context. Should be posix compliant.*/
    while ((int)syscall(SYSCALL_MUTEX_TEST_AND_SET, (void *)(&(mutex->lock)))) {
        req_context_switch();
    }
    return 0;

}

int pthread_mutex_trylock(pthread_mutex_t * mutex)
{
    int result;

    result = (int)syscall(SYSCALL_MUTEX_TEST_AND_SET, (void *)(&(mutex->lock)));
    if (result == 0) {
        mutex->thread_id = pthread_self();
        return 0;
    }
    /*Should we request context switch if not succesful?*/
    return 1;
}

int pthread_mutex_unlock(pthread_mutex_t * mutex)
{
    if (mutex->thread_id == pthread_self()) {
        mutex->lock = 0;
        return 0;
    }
    return 1;
}
