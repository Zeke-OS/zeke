/**
 *******************************************************************************
 * @file    fs.c
 * @author  Olli Vanhoja
 * @brief   Virtual file system.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup Dev
  * @{
  */

#include <kstring.h>
#include <sched.h>
#include <ksignal.h>
#include <syscalldef.h>
#include <syscall.h>
#define KERNEL_INTERNAL 1
#include <errno.h>
#include "fs.h"

fs_t fs_alloc_table[configFS_MAX];


/**
 * fs syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uint32_t fs_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_FS_CREAT:
        current_thread->errno = ENOSYS;
        return -1;

    case SYSCALL_FS_OPEN:
        current_thread->errno = ENOSYS;
        return -2;

    case SYSCALL_FS_CLOSE:
        current_thread->errno = ENOSYS;
        return -3;

    case SYSCALL_FS_READ:
        current_thread->errno = ENOSYS;
        return -4;

    case SYSCALL_FS_WRITE:
        current_thread->errno = ENOSYS;
        return -5;

    case SYSCALL_FS_LSEEK:
        current_thread->errno = ENOSYS;
        return -6;

    case SYSCALL_FS_DUP:
        current_thread->errno = ENOSYS;
        return -7;

    case SYSCALL_FS_LINK:
        current_thread->errno = ENOSYS;
        return -8;

    case SYSCALL_FS_UNLINK:
        current_thread->errno = ENOSYS;
        return -9;

    case SYSCALL_FS_STAT:
        current_thread->errno = ENOSYS;
        return -10;

    case SYSCALL_FS_FSTAT:
        current_thread->errno = ENOSYS;
        return -11;

    case SYSCALL_FS_ACCESS:
        current_thread->errno = ENOSYS;
        return -12;

    case SYSCALL_FS_CHMOD:
        current_thread->errno = ENOSYS;
        return -13;

    case SYSCALL_FS_CHOWN:
        current_thread->errno = ENOSYS;
        return -14;

    case SYSCALL_FS_UMASK:
        current_thread->errno = ENOSYS;
        return -15;

    case SYSCALL_FS_IOCTL:
        current_thread->errno = ENOSYS;
        return -16;

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */
