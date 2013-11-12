/**
 *******************************************************************************
 * @file    syscall.h
 * @author  Olli Vanhoja
 * @brief   Header file for syscalls.
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

#pragma once
#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel.h>
#include <syscalldef.h>
#if configDEVSUBSYS != 0
#include <devtypes.h>
#endif

#define SYSCALL_MINORBITS   27 /*!< Number of minor bits */
#define SYSCALL_MINORMASK   ((1u << MINORBITS) - 1) /*!< Minor bits mask */

/**
 * Get syscall major number from uint32_t.
 */
#define SYSCALL_MAJOR(type) ((uint32_t)((type) >> DEV_MINORBITS))

/**
 * Get syscall minor number from uint32_t.
 */
#define SYSCALL_MINOR(type) ((uint32_t)((type) & DEV_MINORMASK))

/**
 * Convert major, minor pair into syscall type code (uint32_t).
 */
#define SYSCALL_MMTOTYPE(ma, mi) (((ma) << DEV_MINORBITS) | (mi))

/* Syscall groups */
#define SYSCALL_GROUP_SCHED         0x01
#define SYSCALL_GROUP_SCHED_THREAD  0x02
#define SYSCALL_GROUP_SIGNAL        0x03
#define SYSCALL_GROUP_DEV           0x04
#define SYSCALL_GROUP_LOCKS         0x05

/* List of syscalls */
#define SYSCALL_SCHED_THREAD_CREATE         SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x00)
#define SYSCALL_SCHED_THREAD_GETID          SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x01)
#define SYSCALL_SCHED_THREAD_TERMINATE      SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x02)
/* osThreadYield is not a syscall */
#define SYSCALL_SCHED_THREAD_SETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x03)
#define SYSCALL_SCHED_THREAD_GETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x04)
#define SYSCALL_SCHED_THREAD_GETERRNO       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED_THREAD, 0x05)
#define SYSCALL_SCHED_DELAY                 SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x00)
#define SYSCALL_SCHED_WAIT                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x01)
#define SYSCALL_SCHED_GET_LOADAVG           SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x02)
#define SYSCALL_SCHED_EVENT_GET             SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x03)
#define SYSCALL_SIGNAL_SET                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x00)
#define SYSCALL_SIGNAL_CLEAR                SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x01)
#define SYSCALL_SIGNAL_GETCURR              SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x02)
#define SYSCALL_SIGNAL_GET                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x03)
#define SYSCALL_SIGNAL_WAIT                 SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x04)
#define SYSCALL_DEV_OPEN                    SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x00)
#define SYSCALL_DEV_CLOSE                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x01)
#define SYSCALL_DEV_CHECK_RES               SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x02)
#define SYSCALL_DEV_CWRITE                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x03)
#define SYSCALL_DEV_CREAD                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x04)
#define SYSCALL_DEV_BWRITE                  SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x05)
#define SYSCALL_DEV_BREAD                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x06)
#define SYSCALL_DEV_BSEEK                   SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x07)
#define SYSCALL_DEV_WAIT                    SYSCALL_MMTOTYPE(SYSCALL_GROUP_DEV, 0x08)
#define SYSCALL_MUTEX_TEST_AND_SET          SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x00)
#define SYSCALL_SEMAPHORE_WAIT              SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x01)
#define SYSCALL_SEMAPHORE_RELEASE           SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x02)

//#ifndef PU_TEST_BUILD
uint32_t syscall(uint32_t type, void * p);
//#endif /* PU_TEST_BUILD */

/* Kernel scope functions */
#ifdef KERNEL_INTERNAL
#include "syscalldef.h"

uint32_t _intSyscall_handler(uint32_t type, void * p);
#endif /* KERNEL_INTERNAL */

#endif /* SYSCALL_H */
