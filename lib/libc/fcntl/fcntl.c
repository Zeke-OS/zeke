/**
 *******************************************************************************
 * @file    fcntl.c
 * @author  Olli Vanhoja
 * @brief   File control.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdarg.h>
#include <syscall.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int fcntl(int fildes, int cmd, ...)
{
    /* TODO */
    va_list ap;
    struct _fs_fcntl_args args = {
        .fd = fildes,
        .cmd = cmd
    };
    int retval = 0;

    va_start(ap, cmd);
    switch (cmd) {
    case F_DUPFD:
    case F_DUP2FD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
        args.third.ival = va_arg(ap, int);
        break;
    case F_GETLK:
    case F_SETLK:
    case F_SETLKW:
        args.third.fl = va_arg(ap, struct flock);
        break;
    default:
        errno = EINVAL;
        retval = -1;
    }
    va_end(ap);
    if (retval)
        return retval;

    return syscall(SYSCALL_FS_FCNTL, &args);
}
