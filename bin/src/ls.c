/**
 *******************************************************************************
 * @file    ls.c
 * @author  Olli Vanhoja
 * @brief   ls.
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <unistd.h>

char * argv0;
struct {
    int l : 1;
    int a : 1;
} flags;

static void usage(void)
{
    fprintf(stderr, "usage: %s [-la] [dir]\n", argv0);

    exit(EX_USAGE);
}

int main(int argc, char * argv[], char * envp[])
{
    char * path = NULL;
    int ch, fildes, count;
    struct dirent dbuf[10];

    argv0 = argv[0];

    while ((ch = getopt(argc, argv, "la")) != EOF) {
        switch (ch) {
        case 'l':
            flags.l = 1;
            break;
        case 'a':
            flags.a = 1;
            break;
        case '?':
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (*argv)
        path = *argv;
    if (path == NULL || !strcmp(path, ""))
        path = "./";

    fildes = open(path, O_DIRECTORY | O_RDONLY | O_SEARCH);
    if (fildes < 0) {
        fprintf(stderr, "Open failed\n");
        return EX_NOINPUT;
    }

    while ((count = getdents(fildes, (char *)dbuf, sizeof(dbuf))) > 0) {
        for (int i = 0; i < count; i++) {
            if (!flags.a && dbuf[i].d_name[0] == '.')
                continue;

            if (flags.l) {
                struct stat stat;
                char mode[12];

                fstatat(fildes, dbuf[i].d_name, &stat, 0);
                strmode(stat.st_mode, mode);
                printf("% 7u %s %u:%u %s\n",
                         (unsigned)dbuf[i].d_ino, mode,
                         (unsigned)stat.st_uid, (unsigned)stat.st_gid,
                         dbuf[i].d_name);
            } else {
                printf("%s ", dbuf[i].d_name);
            }
        }
    }
    printf("\n");

    close(fildes);
    return 0;
}
