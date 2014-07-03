/**
 *******************************************************************************
 * @file    tish_dir.c
 * @author  Olli Vanhoja
 * @brief   Directory manipulation commands for tish/Zeke.
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

#include <stdlib.h>
#include <sys/types.h>
#include <kstring.h> /* TODO Remove */
#include <unistd.h>
#include <errno.h>
#include <kernel.h>
#include <fcntl.h>
#include <dirent.h>
#include "tish.h"

/* TODO Remove */
#define fprintf(stream, str) write(2, str, strlenn(str, MAX_LEN) + 1)
#define puts(str) fprintf(stderr, str)

//static char invalid_arg[] = "Invalid argument\n";

#define iprintf(indent, fmt, ...) do { char buf[80];                    \
    ksprintf(buf, sizeof(buf), "%*s" fmt, indent, " ", __VA_ARGS__);    \
    kputs(buf);                                                         \
} while (0)
void tish_ls(char ** args)
{
    int fildes, count;
    struct dirent dbuf[10];

    fildes = open("./", O_DIRECTORY | O_RDONLY);
    if (fildes < 0) {
        puts("Opening ./ failed.\n");
        return;
    }
    count = getdents(fildes, (char *)dbuf, sizeof(dbuf));
    if (count < 1) {
        puts("Reading directory entries failed\n");
    }

    for (int i = 0; i < count; i++) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "%s ", dbuf[i].d_name);
        puts(buf);
    }
    puts("\n");

    close(fildes);
}
