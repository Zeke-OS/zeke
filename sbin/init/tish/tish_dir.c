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
#include <sys/wait.h>
#include <kstring.h> /* TODO Remove */
#include <unistd.h>
#include <libkern.h>
#include <errno.h>
#include <pthread.h>
#include <kernel.h>
#include "tish.h"

/* TODO Remove */
#define fprintf(stream, str) write(2, str, strlenn(str, MAX_LEN) + 1)
#define puts(str) fprintf(stderr, str)

//static char invalid_arg[] = "Invalid argument\n";

static void ls(char * path);

void tish_tree(char ** args)
{
    //char * arg = kstrtok(0, DELIMS, args);

    //walk_dirtree(cwd, 0);
}

#define iprintf(indent, fmt, ...) do { char buf[80];                    \
    ksprintf(buf, sizeof(buf), "%*s" fmt, indent, " ", __VA_ARGS__);    \
    kputs(buf);                                                         \
} while (0)
static void ls(char * path)
{
#if 0
    DIR * dirp;
    struct dirent dp;
    dp.d_off = 0x00000000FFFFFFFF; /* TODO initializer? */

    if ((dirp = opendir(".")) == NULL) {
        puts("couldn't open dir");
        return;
    }

    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            puts(dp->d_name);
        }
    } while (dp != NULL);

    if (errno != 0)
        puts("error reading dir");

    closedir(dirp);
#endif
}
