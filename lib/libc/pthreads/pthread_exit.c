/**
 *******************************************************************************
 * @file    pthread_exit.c
 * @author  Olli Vanhoja
 * @brief   pthread cancellation/exit libc code.
 * @section LICENSE
 * Copyright (c) 2013 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * Copyright (c) 2013-2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <pthread.h>
#include <signal.h>
#include <syscall.h>

/**
 * A key for thread local cleanup routines.
 */
pthread_key_t _pthread_cleanup_handler_key;

/*
 * Default handler for SIGCANCEL.
 */
static void pthread_cancel_handler(int signo)
{
    __pthread_key_dtors();

    if (signo != 0) {
        pthread_exit(NULL);
    }
}

/**
 * Execute cleanup routines registered with pthread_cleanup_push().
 * Shall be registered with pthread_key_create().
 */
static void pthread_cleanup_handler(void * nfo)
{
    struct _pthread_cleanup_info * next = nfo;

    while (next) {
        if (next->rtn) {
            next->rtn(next->arg);
        }
        next = next->next;
    }
}

void __pthread_init(void)
{
    /* Register a signal handler to handle signal cancellation functions. */
    signal(SIGCANCEL, pthread_cancel_handler);

    pthread_key_create(&_pthread_cleanup_handler_key, pthread_cleanup_handler);
}

void pthread_exit(void * retval)
{
    pthread_cancel_handler(0);

    (void)syscall(SYSCALL_THREAD_DIE, retval);
    /* Syscall will not return */
}
