/**
 *******************************************************************************
 * @file    ksignal.c
 * @author  Olli Vanhoja
 *
 * @brief   Source file for thread Signal Management in kernel.
 * @section LICENSE
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

#define KERNEL_INTERNAL 1
#include <syscalldef.h>
#include <syscall.h>
#include <errno.h>
#include <sched.h>
#include <timers.h>
#include "ksignal.h"

/* Syscall handlers ***********************************************************/

/**
 * Scheduler signal syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uint32_t ksignal_syscall(uint32_t type, void * p)
{
    switch(type) {
        case SYSCALL_SIGNAL_KILL:
            current_thread->errno = ENOSYS;
            return -3;

        case SYSCALL_SIGNAL_RAISE:
            current_thread->errno = ENOSYS;
            return -4;

        case SYSCALL_SIGNAL_ACTION:
            current_thread->errno = ENOSYS;
            return -5;

        case SYSCALL_SIGNAL_ALTSTACK:
            current_thread->errno = ENOSYS;
            return -6;

        default:
            current_thread->errno = ENOSYS;
            return -1;
    }
}

