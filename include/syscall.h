/**
 *******************************************************************************
 * @file    syscall.h
 * @author  Olli Vanhoja
 * @brief   Header file for syscalls.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup LIBC
 * @{
 */

/**
 * @addtogroup syscall
 * @{
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <autoconf.h>
#include <sys/param.h>
#include <stddef.h>
#include <stdint.h>
#ifdef KERNEL_INTERNAL
#include <../kern/include/hal/core.h>
#endif

#define SYSCALL_MINORBITS   24 /*!< Number of minor bits */
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
#define SYSCALL_GROUP_SCHED     0x1 /*!< Scheduler system call group. */
#define SYSCALL_GROUP_THREAD    0x2 /*!< Thread system call group. */
#define SYSCALL_GROUP_SYSCTL    0x3 /*!< Sysctl system call group. */
#define SYSCALL_GROUP_SIGNAL    0x4 /*!< Signal system call group. */
#define SYSCALL_GROUP_EXEC      0x5 /*!< Exec system call group. */
#define SYSCALL_GROUP_PROC      0x6 /*!< Process system call group. */
#define SYSCALL_GROUP_IPC       0x7 /*!< IPC system call group. */
#define SYSCALL_GROUP_FS        0x8 /*!< File system system call group. */
#define SYSCALL_GROUP_IOCTL     0x9 /*!< ioctl sysctem call group. */
#define SYSCALL_GROUP_SHMEM     0xA /*!< Shared memory system call group. */
#define SYSCALL_GROUP_TIME      0xB /*!< Time system call group. */
#define SYSCALL_GROUP_PRIV      0xC /*!< Pivileges system call group. */

/* List of syscalls */
#define SYSCALL_SCHED_GET_LOADAVG   SYSCALL_MMTOTYPE(SYSCALL_GROUP_SCHED, 0x00)
#define SYSCALL_THREAD_CREATE       SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x00)
#define SYSCALL_THREAD_DIE          SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x01)
#define SYSCALL_THREAD_DETACH       SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x02)
#define SYSCALL_THREAD_JOIN         SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x03)
#define SYSCALL_THREAD_SLEEP_MS     SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x04)
#define SYSCALL_THREAD_SETPOLICY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x05)
#define SYSCALL_THREAD_GETPOLICY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x06)
#define SYSCALL_THREAD_SETPRIORITY  SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x07)
#define SYSCALL_THREAD_GETPRIORITY  SYSCALL_MMTOTYPE(SYSCALL_GROUP_THREAD, 0x08)
#define SYSCALL_SYSCTL_SYSCTL       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SYSCTL, 0x00)
#define SYSCALL_SIGNAL_PKILL        SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x00)
#define SYSCALL_SIGNAL_TKILL        SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x01)
#define SYSCALL_SIGNAL_SIGNAL       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x02)
#define SYSCALL_SIGNAL_ACTION       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x03)
#define SYSCALL_SIGNAL_ALTSTACK     SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x04)
#define SYSCALL_SIGNAL_SIGMASK      SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x05)
#define SYSCALL_SIGNAL_SIGWAIT      SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x06)
#define SYSCALL_SIGNAL_SIGWAITNFO   SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x07)
#define SYSCALL_SIGNAL_SIGSLEEP     SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x08)
#define SYSCALL_SIGNAL_SETRETURN    SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x09)
#define SYSCALL_SIGNAL_RETURN       SYSCALL_MMTOTYPE(SYSCALL_GROUP_SIGNAL, 0x0A)
#define SYSCALL_EXEC_EXEC           SYSCALL_MMTOTYPE(SYSCALL_GROUP_EXEC, 0x00)
#define SYSCALL_PROC_FORK           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x00)
#define SYSCALL_PROC_WAIT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x01)
#define SYSCALL_PROC_EXIT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x02)
#define SYSCALL_PROC_CRED           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x03)
#define SYSCALL_PROC_GETGROUPS      SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x04)
#define SYSCALL_PROC_SETGROUPS      SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x05)
#define SYSCALL_PROC_GETSID         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x06)
#define SYSCALL_PROC_SETSID         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x07)
#define SYSCALL_PROC_GETPGRP        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x08)
#define SYSCALL_PROC_SETPGID        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x09)
#define SYSCALL_PROC_GETLOGIN       SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x0A)
#define SYSCALL_PROC_SETLOGIN       SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x0B)
#define SYSCALL_PROC_GETPID         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x0C)
#define SYSCALL_PROC_GETPPID        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x0D)
#define SYSCALL_PROC_CHDIR          SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x0E)
#define SYSCALL_PROC_CHROOT         SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x0F)
#define SYSCALL_PROC_SETPOLICY      SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x10)
#define SYSCALL_PROC_GETPOLICY      SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x11)
#define SYSCALL_PROC_SETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x12)
#define SYSCALL_PROC_GETPRIORITY    SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x13)
#define SYSCALL_PROC_GETRLIM        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x14)
#define SYSCALL_PROC_SETRLIM        SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x15)
#define SYSCALL_PROC_TIMES          SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x16)
#define SYSCALL_PROC_GETBREAK       SYSCALL_MMTOTYPE(SYSCALL_GROUP_PROC, 0x17)
#define SYSCALL_IPC_PIPE            SYSCALL_MMTOTYPE(SYSCALL_GROUP_IPC, 0x00)
#define SYSCALL_FS_OPEN             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x00)
#define SYSCALL_FS_CLOSE            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x01)
#define SYSCALL_FS_CLOSE_ALL        SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x02)
#define SYSCALL_FS_READ             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x03)
#define SYSCALL_FS_WRITE            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x04)
#define SYSCALL_FS_LSEEK            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x05)
#define SYSCALL_FS_GETDENTS         SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x06)
#define SYSCALL_FS_FCNTL            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x07)
#define SYSCALL_FS_LINK             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x08)
#define SYSCALL_FS_UNLINK           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x09)
#define SYSCALL_FS_MKDIR            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x0A)
#define SYSCALL_FS_RMDIR            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x0B)
#define SYSCALL_FS_STAT             SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x0C)
#define SYSCALL_FS_STATFS           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x0D)
#define SYSCALL_FS_GETFSSTAT        SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x0E)
#define SYSCALL_FS_ACCESS           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x0F)
#define SYSCALL_FS_UTIMES           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x10)
#define SYSCALL_FS_CHMOD            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x11)
#define SYSCALL_FS_CHFLAGS          SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x12)
#define SYSCALL_FS_CHOWN            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x13)
#define SYSCALL_FS_UMASK            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x14)
#define SYSCALL_FS_MOUNT            SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x15)
#define SYSCALL_FS_UMOUNT           SYSCALL_MMTOTYPE(SYSCALL_GROUP_FS, 0x16)
#define SYSCALL_IOCTL_GETSET        SYSCALL_MMTOTYPE(SYSCALL_GROUP_IOCTL, 0x00)
#define SYSCALL_SHMEM_MMAP          SYSCALL_MMTOTYPE(SYSCALL_GROUP_SHMEM, 0x00)
#define SYSCALL_SHMEM_MUNMAP        SYSCALL_MMTOTYPE(SYSCALL_GROUP_SHMEM, 0x01)
#define SYSCALL_TIME_GETTIME        SYSCALL_MMTOTYPE(SYSCALL_GROUP_TIME, 0x00)
#define SYSCALL_TIME_SETTIME        SYSCALL_MMTOTYPE(SYSCALL_GROUP_TIME, 0x01)
#define SYSCALL_PRIV_PCAP           SYSCALL_MMTOTYPE(SYSCALL_GROUP_PRIV, 0x00)
#define SYSCALL_PRIV_PCAP_GETALL    SYSCALL_MMTOTYPE(SYSCALL_GROUP_PRIV, 0x01)

/* Kernel scope */
#ifdef KERNEL_INTERNAL
typedef intptr_t (*kernel_syscall_handler_t)(uint32_t type, __user void * p);
typedef intptr_t (*syscall_handler_t)(__user void * p);
#define ARRDECL_SYSCALL_HNDL(SYSCALL_NR, fn) [SYSCALL_MINOR(SYSCALL_NR)] = fn

/**
 * Define a syscall handler that calls functions from a function pointer array.
 */
#define SYSCALL_HANDLERDEF(groupfnname, callmaparray)                       \
    intptr_t groupfnname(uint32_t type, __user void * p) {                  \
        uint32_t minor = SYSCALL_MINOR(type);                               \
                                                                            \
        if ((minor >= num_elem(callmaparray)) || !(callmaparray)[minor]) {  \
            set_errno(ENOSYS);                                              \
            return -1;                                                      \
        }                                                                   \
        return (callmaparray)[minor](p);                                    \
}


void syscall_handler(void);
#else /* !KERNEL_INTERNAL */

/**
 * Make a system call.
 * @param type is the system call code.
 * @param p is pointer to a syscall args struct or to a single argument.
 * @return Return value of the called kernel function.
 * @note Must be only used in thread scope.
 */
intptr_t syscall(uint32_t type, void * p);

#include <machine/mach_syscall.h>

#endif /* !KERNEL_INTERNAL */

#endif /* SYSCALL_H */

/**
 * @}
 */

/**
 * @}
 */
