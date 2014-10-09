/**
 *******************************************************************************
 * @file    pthread.h
 * @author  Olli Vanhoja
 * @brief   Threads.
 * @section LICENSE
 * Copyright (c) 2013 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *
 *
 * Copyright (c) 1993, 1994 by Chris Provenzano, proven@mit.edu
 * Copyright (c) 1995-1998 by John Birrell <jb@cimlogic.com.au>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Chris Provenzano.
 * 4. The name of Chris Provenzano may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CHRIS PROVENZANO ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CHRIS PROVENZANO BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *******************************************************************************
 */

/** @addtogroup LIBC
 * @{
 */

#pragma once
#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/cdefs.h>
#include <sys/types_pthread.h>
#include <sched.h>
#include <time.h>

/** @addtogroup Threads
 * @{
 */

/* TODO Missing most of the standard declarations */
/* TODO errnos */

/*
 * Run-time invariant values:
 */
#define PTHREAD_DESTRUCTOR_ITERATIONS       4
#define PTHREAD_KEYS_MAX            256
#define PTHREAD_STACK_MIN           __MINSIGSTKSZ
#define PTHREAD_THREADS_MAX         __ULONG_MAX
#define PTHREAD_BARRIER_SERIAL_THREAD       -1

/*
 * Flags for threads and thread attributes.
 */
#define PTHREAD_DETACHED            0x1
#define PTHREAD_SCOPE_SYSTEM        0x2
#define PTHREAD_INHERIT_SCHED       0x4
#define PTHREAD_NOFLOAT             0x8

#define PTHREAD_CREATE_DETACHED     PTHREAD_DETACHED
#define PTHREAD_CREATE_JOINABLE     0
#define PTHREAD_SCOPE_PROCESS       0
#define PTHREAD_EXPLICIT_SCHED      0

/*
 * Flags for read/write lock attributes
 */
#define PTHREAD_PROCESS_PRIVATE     0
#define PTHREAD_PROCESS_SHARED      1

/*
 * Flags for cancelling threads
 */
#define PTHREAD_CANCEL_ENABLE       0
#define PTHREAD_CANCEL_DISABLE      1
#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 2
#define PTHREAD_CANCELED        ((void *) 1)

/*
 * Flags for once initialization.
 */
#define PTHREAD_NEEDS_INIT  0
#define PTHREAD_DONE_INIT   1

/*
 * Static once initialization values.
 */
#define PTHREAD_ONCE_INIT   { PTHREAD_NEEDS_INIT, NULL }

/*
 * Static initialization values.
 */
#define PTHREAD_MUTEX_INITIALIZER   NULL
#define PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP   ((pthread_mutex_t)1)
#define PTHREAD_COND_INITIALIZER    NULL
#define PTHREAD_RWLOCK_INITIALIZER  NULL

/*
 * Default attribute arguments (draft 4, deprecated).
 */
#ifndef PTHREAD_KERNEL
#define pthread_condattr_default    NULL
#define pthread_mutexattr_default   NULL
#define pthread_attr_default        NULL
#endif

#define PTHREAD_PRIO_NONE   0
#define PTHREAD_PRIO_INHERIT    1
#define PTHREAD_PRIO_PROTECT    2

/*
 * Mutex types (Single UNIX Specification, Version 2, 1997).
 *
 * Note that a mutex attribute with one of the following types:
 *
 *  PTHREAD_MUTEX_NORMAL
 *  PTHREAD_MUTEX_RECURSIVE
 *
 * will deviate from POSIX specified semantics.
 */
enum pthread_mutextype {
    PTHREAD_MUTEX_ERRORCHECK    = 1,    /* Default POSIX mutex */
    PTHREAD_MUTEX_RECURSIVE     = 2,    /* Recursive mutex */
    PTHREAD_MUTEX_NORMAL        = 3,    /* No error checking */
    PTHREAD_MUTEX_ADAPTIVE_NP   = 4,    /* Adaptive mutex, spins briefly before blocking on lock */
    PTHREAD_MUTEX_TYPE_MAX
};

#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_ERRORCHECK

struct _pthread_cleanup_info {
    uintptr_t pthread_cleanup_pad[8];
};

typedef int pthread_key_t;
typedef struct pthread_once pthread_once_t;

__BEGIN_DECLS
/**
 * Get calling thread's ID.
 *
 * The pthread_self() function returns the thread ID of the calling thread.
 * @return The thread ID of the calling thread.
 */
pthread_t pthread_self(void);

/**
 * Create a thread and add it to Active Threads and set it to state READY.
 * @param thread[out] returns ID of the new thread.
 * @param attr thread creation attributes.
 * @param start_routine
 * @param arg
 * @return If successful, the pthread_create() function returns zero. Otherwise,
 *         an error number is returned to indicate the error.
 */
int pthread_create(pthread_t * thread, const pthread_attr_t * attr,
        void * (*start_routine)(void *), void * arg);

/**
 * Thread termination.
 * @param retval is the return value of the thread.
 * @return This function does not return to the caller.
 */
void pthread_exit(void * retval);

/**
 * Indicate that storage for the thread thread can be reclaimed when that
 * thread terminates.
 * Calling this function will not terminate the thread if it's not already
 * terminated.
 */
int pthread_detach(pthread_t thread);


/**
 * Initialises the mutex referenced by mutex with attributes specified by attr.
 * If attr is NULL, the default mutex attributes are used.
 * @param mutex is a pointer to the mutex control block.
 * @param attr is a struct containing mutex attributes.
 * @return If successful return zero; Otherwise value other than zero.
 */
int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr);

/**
 * Lock mutex.
 * If the mutex is already locked, the calling thread blocks until the mutex
 * becomes available. This operation returns with the mutex object referenced by
 * mutex in the locked state with the calling thread as its owner.
 * @param mutex is the mutex control block.
 * @return If successful return zero; Otherwise value other than zero.
 */
int pthread_mutex_lock(pthread_mutex_t * mutex);

/**
 * Try to lock mutex and return if can't acquire lock due to it is locked by
 * any thread including the current thread.
 * @param mutex is the mutex control block.
 * @return If successful return zero; Otherwise value other than zero.
 * @exception EBUSY The mutex could not be acquired because it was already locked.
 */
int pthread_mutex_trylock(pthread_mutex_t * mutex);

/**
 * Release the mutex object.
 * @param mutex is the mutex control block.
 * @return If successful return zero; Otherwise value other than zero.
 */
int pthread_mutex_unlock(pthread_mutex_t * mutex);


/* TODO Legacy */

/// Terminate execution of a thread and remove it from Active Threads.
/// \param[in]     thread_id   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return status code that indicates the execution status of the function.
int osThreadTerminate(pthread_t thread_id);
__END_DECLS

#endif /* PTHREAD_H */

/**
 * @}
 */

/**
 * @}
 */
