/**
 *******************************************************************************
 * @file    ioctl.h
 * @author  Olli Vanhoja
 * @brief   Control devices.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef IOCTL_H
#define IOCTL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

/*
 * IOCTL request codes.
 * Get requests shall be odd and set request shall be even, this information can
 * be then used to optimize the syscall.
 */
/* termio */
#define IOCTL_GTERMIOS       1  /*!< Get termios struct. */
#define IOCTL_STERMIOS       2  /*!< Set termios struct. */
/* dev */
#define IOCTL_GETBLKSIZE    10  /*!< Get device block size. */
#define IOCTL_GETBLKCNT     11  /*!< Get device block count. */
/* pty */
#define IOCTL_PTY_CREAT     50  /*!< Create a new pty master-slave pair. */

struct _ioctl_get_args {
    int fd;
    unsigned request;
    void * arg;
    size_t arg_len;
};

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

/**
 * ioctl.
 * @note This is a non-POSIX implementation of ioctl.
 */
int _ioctl(int fildes, unsigned request, void * arg, size_t arg_len);

__END_DECLS
#endif

#endif /* IOCTL_H */

/**
 * @}
 */
