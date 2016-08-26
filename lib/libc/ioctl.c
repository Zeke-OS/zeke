/**
 *******************************************************************************
 * @file    ioctl.c
 * @author  Olli Vanhoja
 * @brief   ioctl libc functions.
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

#include <errno.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/fb.h>
#include <syscall.h>
#include <termios.h>

int ioctl(int fildes, int request, ... /* arg */)
{
    va_list ap;
    void * arg;
    size_t arg_len;

    va_start(ap, request);
    switch (request) {
    case FIONREAD:
    case FIONWRITE:
    case FIONSPACE:
        arg = va_arg(ap, int *);
        arg_len = sizeof(int *);
        break;
    case TCGETS:
        return tcgetattr(fildes, va_arg(ap, struct termios *));
    case TCSETS:
        return tcsetattr(fildes, TCSANOW, va_arg(ap, struct termios *));
    case TCSETSW:
        return tcsetattr(fildes, TCSADRAIN, va_arg(ap, struct termios *));
    case TCSETSF:
        return tcsetattr(fildes, TCSAFLUSH, va_arg(ap, struct termios *));
    case IOCTL_FB_GETRES:
    case IOCTL_FB_SETRES:
        arg = va_arg(ap, struct fb_resolution *);
        arg_len = sizeof(struct fb_resolution);
        break;
    case TIOCGWINSZ:
    case TIOCSWINSZ:
        arg = va_arg(ap, struct winsize *);
        arg_len = sizeof(struct winsize);
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    va_end(ap);

    _ioctl(fildes, request, arg, arg_len);

    return 0;
}
