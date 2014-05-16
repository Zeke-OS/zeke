/**
 *******************************************************************************
 * @file    unistd.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
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

/** @addtogroup Library_Functions
  * @{
  */

#include <syscall.h>
#include <unistd.h>

/** @addtogroup unistd
 *  @{
 */

pid_t fork(void)
{
    return (pid_t)syscall(SYSCALL_PROC_FORK, NULL);
}

ssize_t pwrite(int fildes, const void * buf, size_t nbyte,
        off_t offset)
{
    struct _fs_write_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbyte = nbyte,
        .offset = offset
    };

    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}

ssize_t write(int fildes, const void * buf, size_t nbyte)
{
    struct _fs_write_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbyte = nbyte,
        .offset = SEEK_CUR
    };

    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}

unsigned sleep(unsigned seconds)
{
    unsigned int millisec = seconds * 1000;

    return (unsigned)syscall(SYSCALL_SCHED_SLEEP_MS, &millisec);
}


/**
  * @}
  */

/**
  * @}
  */
