/**
 *******************************************************************************
 * @file    init.c
 * @author  Olli Vanhoja
 * @brief   First user scope process.
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

#include <autoconf.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mount.h>
#include "tish/tish.h"
#include "init.h"

char banner[] = "\
|'''''||                    \n\
    .|'   ...'||            \n\
   ||   .|...|||  ..  ....  \n\
 .|'    ||    || .' .|...|| \n\
||......|'|...||'|. ||      \n\
             .||. ||.'|...'\n\n\
";

static const char msg[] = "Zeke " KERNEL_VERSION " init\n";

void * main(void * arg)
{
    int r0, r1, r2;
    const char tty_path[] = "/dev/ttyS0";
//    char buf[80];

    mkdir("/dev", S_IRWXU | S_IRGRP | S_IXGRP);
    mount("", "/dev", "devfs", 0, "");

    mkdir("/proc", S_IRWXU | S_IRGRP | S_IXGRP);
    mount("", "/proc", "procfs", 0, "");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    r0 = open(tty_path, O_RDONLY);
    r1 = open(tty_path, O_WRONLY);
    r2 = open(tty_path, O_WRONLY);

#if 0
    ksprintf(buf, sizeof(buf), "fd: %i, %i, %i\n", r0, r1, r2);
    write(STDOUT_FILENO, buf, strlenn(buf, sizeof(buf)));
#endif

    write(STDOUT_FILENO, banner, sizeof(banner));
    write(STDOUT_FILENO, msg, sizeof(msg));

#if configTISH != 0
    tish();
#endif
    while(1) {
        write(STDOUT_FILENO, "init\n", 5);
        sleep(10);
    }
}
