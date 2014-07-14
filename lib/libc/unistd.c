/**
 *******************************************************************************
 * @file    unistd.c
 * @author  Olli Vanhoja
 * @brief   Standard functions.
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

#include <syscall.h>
#include <unistd.h>

pid_t fork(void)
{
    return (pid_t)syscall(SYSCALL_PROC_FORK, NULL);
}

ssize_t read(int fildes, void * buf, size_t nbytes)
{
    struct _fs_readwrite_args args = {
        .fildes = fildes,
        .buf = buf,
        .nbytes = nbytes
    };

    return (ssize_t)syscall(SYSCALL_FS_READ, &args);
}

#if 0
ssize_t pwrite(int fildes, const void * buf, size_t nbyte,
        off_t offset)
{
    struct _fs_readwrite_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbytes = nbyte
    };

    /* TODO Seek first */
    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}
#endif

ssize_t write(int fildes, const void * buf, size_t nbyte)
{
    struct _fs_readwrite_args args = {
        .fildes = fildes,
        .buf = (void *)buf,
        .nbytes = nbyte
    };

    return (ssize_t)syscall(SYSCALL_FS_WRITE, &args);
}

off_t lseek(int fildes, off_t offset, int whence)
{
    int err;
    struct _fs_lseek_args args = {
        .fd = fildes,
        .offset = offset,
        .whence = whence
    };

    err = (int)syscall(SYSCALL_FS_LSEEK, &args);
    if (err)
        return -1;

    return args.offset;
}
