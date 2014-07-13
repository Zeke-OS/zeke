/**
 *******************************************************************************
 * @file    termios.c
 * @author  Olli Vanhoja
 * @brief   Termios.
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

#include <syscall.h>
#include <sys/ioctl.h>
#include <termios.h>

speed_t cfgetispeed(const struct termios * termios_p)
{
    return termios_p->c_ispeed;
}

speed_t cfgetospeed(const struct termios * termios_p)
{
    return termios_p->c_ospeed;
}

int cfsetispeed(struct termios * termios_p, speed_t speed)
{
    termios_p->c_ispeed = speed;
    return 0;
}

int cfsetospeed(struct termios * termios_p, speed_t speed)
{
    termios_p->c_ospeed = speed;
    return 0;
}

int tcgetattr(int fildes, struct termios * termios_p)
{
    return _ioctl(fildes, IOCTL_GTERMIOS, termios_p, sizeof(struct termios));
}

int tcsetattr(int fildes, int optional_actions,
        const struct termios * termios_p)
{
    return _ioctl(fildes, IOCTL_STERMIOS, (void *)termios_p, sizeof(struct termios));
}
