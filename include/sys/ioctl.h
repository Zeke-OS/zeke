/**
 *******************************************************************************
 * @file    ioctl.h
 * @author  Olli Vanhoja
 * @brief   Control devices.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup ioctl
 * @{
 */

#ifndef IOCTL_H
#define IOCTL_H

#include <stddef.h>
#include <stdint.h>

/*
 * IOCTL request codes.
 * Get requests shall be odd and set request shall be even, this information can
 * be then used to optimize the syscall.
 * @{
 */
/* generic */
#define FIONREAD            1 /*!< Get the number of bytes ready for reading. */
#define FIONWRITE           3 /*!< Get the number of bytes in send queue. */
#define FIONSPACE           5 /*!< Get the number of bytes free in send queue. */
/* termio */
#define IOCTL_GTERMIOS     11 /*!< Get termios struct. */
#define IOCTL_STERMIOS     12 /*!< Set termios struct. */
#define IOCTL_TTYFLUSH     13 /*!< TTY flush controls. */
#define IOCTL_TCSBRK       14 /*!< Send a break. */
/* dev */
#define IOCTL_GETBLKSIZE   21 /*!< Get device block size. */
#define IOCTL_GETBLKCNT    23 /*!< Get device block count. */
#define IOCTL_FLSBLKBUF    24 /*1< Flush block device buffers. */
/* pty */
#define IOCTL_PTY_CREAT    50 /*!< Create a new pty master-slave pair. */
/* dev/fb */
#define IOCTL_FB_GETRES   101 /*!< Get the frame buffer resolution. */
#define IOCTL_FB_SETRES   102 /*!< Change the framebuffer resolution. */
/* Get and set window size */
#define IOCTL_TIOCGWINSZ  103 /*!< Get window size. */
#define IOCTL_TIOCSWINSZ  104 /*!< Set window size. */
/**
 * @}
 */

/* Linux compat */
#ifndef KERNEL_INTERNAL

/* Get and set terminal attributes */
#define TCGETS              0x5401
#define TCSETS              0x5402
#define TCSETSW             0x5403
#define TCSETSF             0x5404

/* Buffer count and flushing */
#define TIOCINQ             FIONREAD
#define TCFLSH              0x540B

/* Get and set window size */
#define TIOCGWINSZ IOCTL_TIOCGWINSZ /*!< Get window size. */
#define TIOCSWINSZ IOCTL_TIOCSWINSZ /*!< Set window size. */
#endif

struct winsize {
   unsigned short ws_row;
   unsigned short ws_col;
   unsigned short ws_xpixel;
   unsigned short ws_ypixel;
};

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/** Args for ioctl syscall */
struct _ioctl_get_args {
    int fd;
    unsigned request;
    void * arg;
    size_t arg_len;
};
#endif

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS

/**
 * Control device.
 * @{
 */

/**
 * ioctl.
 * @note This is a non-POSIX implementation of ioctl.
 */
int _ioctl(int fildes, unsigned request, void * arg, size_t arg_len);

/**
 * ioctl.
 */
int ioctl(int fildes, int request, ... /* arg */);

/**
 * @}
 */

__END_DECLS
#endif

#endif /* IOCTL_H */

/**
 * @}
 */

/**
 * @}
 */
