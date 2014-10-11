/*
 * pthread_mutex.c
 *
 * POSIX mutexes
 *
 * Copyright (C) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <machine/atomic.h>
#include <signal.h>
#include <pthread.h>

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    if (!attr)
        return EINVAL;

    attr->pshared = PTHREAD_PROCESS_PRIVATE;
    attr->kind = PTHREAD_MUTEX_DEFAULT;

    return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0;
}

int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared)
{
    if (!attr || !pshared)
        return EINVAL;

    *pshared = attr->pshared;

    return 0;
}

int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared)
{
    if (!attr)
        return EINVAL;

    if (pshared != PTHREAD_PROCESS_PRIVATE && pshared != PTHREAD_PROCESS_SHARED)
        return EINVAL;

    attr->pshared = pshared;

    return 0;
}

int pthread_mutexattr_gettype(pthread_mutexattr_t *attr, int *kind)
{
    if (!attr || !kind)
        return EINVAL;

    *kind = attr->kind;

    return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind)
{
    if (!attr)
        return EINVAL;

    if (kind != PTHREAD_MUTEX_NORMAL &&
        kind != PTHREAD_MUTEX_RECURSIVE &&
        kind != PTHREAD_MUTEX_ERRORCHECK)
        return EINVAL;

    attr->kind = kind;
    return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    if (!mutex)
        return EINVAL;

    if (attr && attr->pshared == PTHREAD_PROCESS_SHARED)
        return ENOSYS;

    mutex->lock = 0;
    mutex->recursion = 0;
    mutex->kind = attr ? attr->kind : PTHREAD_MUTEX_DEFAULT;
    mutex->owner = -1;
    sigemptyset(&mutex->sigset);
    sigaddset(&mutex->sigset, SIGCONT);

    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex) {
        return EINVAL;
    }

  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (mutex->kind == PTHREAD_MUTEX_NORMAL) {
        if (atomic_set(&mutex->lock, 1) != 0) {
            while (atomic_set(&mutex->lock, -1) != 0) {
                int sig;
                if (sigwait(&mutex->sigset, &sig) != 0)
                    return EINVAL;
            }
        }
    } else {
        pthread_t self = pthread_self();

        if (atomic_set(&mutex->lock, 1) == 0) {
            mutex->recursion = 1;
            mutex->owner = self;
        } else {
            if (pthread_equal(mutex->owner, self)) {
                if (mutex->kind == PTHREAD_MUTEX_RECURSIVE)
                    mutex->recursion++;
                else
                    return EDEADLK;
            } else {
                while (atomic_set(&mutex->lock, -1) != 0) {
                    int sig;
                    if (sigwait(mutex->sigset, &sig) != 0)
                        return EINVAL;
                    mutex->recursion = 1;
                    mutex->owner = self;
                }
            }
        }
    }

    return 0;
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            const struct timespec *abstime)
{
    if (mutex->kind == PTHREAD_MUTEX_NORMAL) {
        if (atomic_set(&mutex->lock, 1) != 0) {
            while (atomic_set(&mutex->lock, -1) != 0) {
                siginfo_t info;
                if (sigtimedwait(&mutex->sigset, &info, abstime) != 0)
                    return EINVAL;
            }
        }
    } else {
        pthread_t self = pthread_self();

        if (atomic_set(&mutex->lock, 1) == 0) {
            mutex->recursion = 1;
            mutex->owner = self;
        } else {
            if (pthread_equal(mutex->owner, self)) {
                if (mutex->kind == PTHREAD_MUTEX_RECURSIVE)
                    mutex->recursion++;
                else
                    return EDEADLK;
            } else {
                while (atomic_set(&mutex->lock, -1) != 0) {
                    siginfo_t info;
                    if (sigtimedwait(&mutex->sigset, &info, abstime) != 0)
                        return EINVAL;
                    mutex->recursion = 1;
                    mutex->owner = self;
                }
            }
        }
    }

    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (atomic_cmpxchg(&mutex->lock, 0, 1) == 0) {
        if (mutex->kind != PTHREAD_MUTEX_NORMAL) {
            mutex->recursion = 1;
            mutex->owner = pthread_self();
        }
    } else {
        if (mutex->kind == PTHREAD_MUTEX_RECURSIVE &&
                pthread_equal(mutex->owner, pthread_self()))
            mutex->recursion++;
        else
            return EBUSY;
    }

    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (mutex->kind == PTHREAD_MUTEX_NORMAL) {
        long idx;

        idx = atomic_set(&mutex->lock, 0);
        if (idx != 0) {
            if (idx < 0) {
                /* Signal to all threads of the current process */
                if (pthread_kill(-2, SIGCONT))
                    return EINVAL;
            }
        } else {
            return EPERM;
        }
    } else {
        if (pthread_equal(mutex->owner, pthread_self())) {
            if (mutex->kind != PTHREAD_MUTEX_RECURSIVE ||
                    --mutex->recursion == 0) {
                mutex->owner = -1;
                if (atomic_set(&mutex->lock, 0) < 0) {
                    /* Signal to all threads of the current process */
                    if (pthread_kill(-2, SIGCONT))
                        return EINVAL;
                }
            }
        } else {
            return EPERM;
        }
    }

    return 0;
}
