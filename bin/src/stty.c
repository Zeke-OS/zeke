/**
 *******************************************************************************
 * @file    stty.c
 * @author  Olli Vanhoja
 * @brief   stty.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define PRINTFLAG(x, name, flg) \
    if ((x) & (flg)) printf(" %s", name);

int main(int argc, char * argv[], char * envp[])
{
    struct termios t;

    tcgetattr(1, &t);

#define FORALL_IFLAGS(apply) \
    apply(BRKINT) \
    apply(ICRNL) \
    apply(IGNBRK) \
    apply(IGNCR) \
    apply(IGNPAR) \
    apply(INLCR) \
    apply(INPCK) \
    apply(ISTRIP) \
    apply(IXANY) \
    apply(IXOFF) \
    apply(IXON) \
    apply(PARMRK)

#define FORALL_OFLAGS(apply) \
    apply(OPOST) \
    apply(ONLCR) \
    apply(OCRNL) \
    apply(ONOCR) \
    apply(ONLRET) \
    apply(OFDEL)

#define FORALL_CFLAGS(apply) \
    apply(CIGNORE) \
    apply(CSIZE) \
    apply(CS5) \
    apply(CS6) \
    apply(CS7) \
    apply(CS8) \
    apply(CSTOPB) \
    apply(CREAD) \
    apply(PARENB) \
    apply(PARODD) \
    apply(HUPCL) \
    apply(CLOCAL)

#define FORALL_LFLAGS(apply) \
    apply(ECHO) \
    apply(ECHOE) \
    apply(ECHOK) \
    apply(ECHONL) \
    apply(ICANON) \
    apply(IEXTEN) \
    apply(ISIG) \
    apply(NOFLSH) \
    apply(TOSTOP)
    /*apply(DEFECHO)*/

#define PIFLAGS(flag) \
    PRINTFLAG(t.c_iflag, #flag, flag)

#define POFLAGS(flag) \
    PRINTFLAG(t.c_oflag, #flag, flag)

#define PCFLAGS(flag) \
    PRINTFLAG(t.c_cflag, #flag, flag)

#define PLFLAGS(flag) \
    PRINTFLAG(t.c_lflag, #flag, flag)

    printf("t.c_iflag:");
    FORALL_IFLAGS(PIFLAGS)

    printf("\nt.c_oflag:");
    FORALL_OFLAGS(POFLAGS)

    printf("\nt.c_cflag:");
    FORALL_CFLAGS(PCFLAGS)

    printf("\nt.c_lflag:");
    FORALL_LFLAGS(PLFLAGS)

    printf("\nispeed: %d\n", cfgetispeed(&t));
    printf("ospeed: %d\n", cfgetospeed(&t));

    return 0;
}
