/**
 *******************************************************************************
 * @file    syscall.h
 * @author  Olli Vanhoja
 * @brief   Header file for syscalls.
 * @section LICENSE
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
 *******************************************************************************
 */

/** @addtogroup LIBC
 * @{
 */


#pragma once
#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel.h>
#include <syscalldef.h>
#include <../kern/include/hal/core.h>
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
#define SYSCALL_GROUP_THREAD        0x02
#define SYSCALL_GROUP_SYSCTL        0x03
#define SYSCALL_GROUP_SIGNAL        0x04
#define SYSCALL_GROUP_PROC          0x05
#define SYSCALL_GROUP_IPC           0x06
#define SYSCALL_GROUP_FS            0x07
#define SYSCALL_GROUP_LOCKS         0x08

/* List of syscalls */
#define SYSCALL_SCHED_DIE           SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x00)
#define SYSCALL_SCHED_DETACH        SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x01)
#define SYSCALL_SCHED_SETPRIORITY   SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x02)
#define SYSCALL_SCHED_GETPRIORITY   SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x03)
#define SYSCALL_SCHED_SLEEP_MS      SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x04)
#define SYSCALL_SCHED_GET_LOADAVG   SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x05)
#define SYSCALL_THREAD_CREATE       SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x00)
#define SYSCALL_THREAD_TERMINATE    SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x01)
#define SYSCALL_THREAD_GETTID       SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x02)
#define SYSCALL_THREAD_GETERRNO     SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x03)
#define SYSCALL_SYSCTL_SYSCTL       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SYSCTL, 0x01)
#define SYSCALL_SIGNAL_KILL         SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x00)
#define SYSCALL_SIGNAL_RAISE        SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x01)
#define SYSCALL_SIGNAL_ACTION       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x02)
#define SYSCALL_SIGNAL_ALTSTACK     SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x03)
#define SYSCALL_PROC_EXEC           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x00)
#define SYSCALL_PROC_FORK           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x01)
#define SYSCALL_PROC_WAIT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x02)
#define SYSCALL_PROC_EXIT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x03)
#define SYSCALL_PROC_GETUID         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x04)
#define SYSCALL_PROC_GETEUID        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x05)
#define SYSCALL_PROC_GETGID         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x06)
#define SYSCALL_PROC_GETEGID        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x07)
#define SYSCALL_PROC_GETPID         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x08)
#define SYSCALL_PROC_GETPPID        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x09)
#define SYSCALL_PROC_ALARM          SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x10)
#define SYSCALL_PROC_CHDIR          SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x11)
#define SYSCALL_PROC_SETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x12)
#define SYSCALL_PROC_GETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x13)
#define SYSCALL_PROC_GETBREAK       SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x14)
#define SYSCALL_IPC_PIPE            SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x00)
#define SYSCALL_IPC_MSGGET          SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x01)
#define SYSCALL_IPC_MSGSND          SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x02)
#define SYSCALL_IPC_MSGRCV          SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x03)
#define SYSCALL_IPC_MSGCTL          SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x04)
#define SYSCALL_IPC_SEMGET          SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x05)
#define SYSCALL_IPC_SEMOP           SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x06)
#define SYSCALL_IPC_SHMGET          SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x07)
#define SYSCALL_IPC_SHMAT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x08)
#define SYSCALL_IPC_SHMDT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x09)
#define SYSCALL_FS_CREAT            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x00)
#define SYSCALL_FS_OPEN             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x01)
#define SYSCALL_FS_CLOSE            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x02)
#define SYSCALL_FS_READ             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x03)
#define SYSCALL_FS_WRITE            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x04)
#define SYSCALL_FS_LSEEK            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x05)
#define SYSCALL_FS_DUP              SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x06)
#define SYSCALL_FS_LINK             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x07)
#define SYSCALL_FS_UNLINK           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x08)
#define SYSCALL_FS_STAT             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x09)
#define SYSCALL_FS_FSTAT            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x10)
#define SYSCALL_FS_ACCESS           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x11)
#define SYSCALL_FS_CHMOD            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x12)
#define SYSCALL_FS_CHOWN            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x13)
#define SYSCALL_FS_UMASK            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x14)
#define SYSCALL_FS_IOCTL            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x15)
#define SYSCALL_FS_MOUNT            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x16)
#define SYSCALL_MUTEX_TEST_AND_SET  SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x00)
#define SYSCALL_SEMAPHORE_WAIT      SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x01)
#define SYSCALL_SEMAPHORE_RELEASE   SYSCALL_MMTOTYPE(SYSCALL_GROUP_LOCKS, 0x02)

/* Kernel scope functions */
#ifdef KERNEL_INTERNAL
#include "syscalldef.h"

typedef uintptr_t (*kernel_syscall_handler_t)(uint32_t type,  void * p);

void syscall_handler(void);
#else /* !KERNEL_INTERNAL */
/**
 * Make a system call
 * @param type syscall type.
 * @param p pointer to the syscall parameter(s).
 * @return return value of the called kernel function.
 * @note Must be only used in thread scope.
 */
uint32_t syscall(uint32_t type, void * p);

#if configARCH == __ARM6__ || configARCH == __ARM6K__
/**
 * Request immediate context switch
 *
 * Called from thread context.
 */
#define req_context_switch() do {           \
    __asm__ volatile ("WFI");               \
} while (0)
#elif configARCH == __ARM6M__
/**
 * Request immediate context switch
 */
#define req_context_switch() do {                                              \
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* Switch the context */              \
    __asm__ volatile ("DSB\n\t" /* Ensure write is completed
                               * (architecturally required, but not strictly
                               * required for existing Cortex-M processors) */ \
                      "ISB\n" /* Ensure PendSV is executed */                  \
    );                                                                         \
} while (0)
#else
#error Selected core is not suported by this libc
#endif

#endif /* !KERNEL_INTERNAL */

#endif /* SYSCALL_H */

/**
 * @}
 */

