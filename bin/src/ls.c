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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

int main(int argc, char * argv[], char * envp[])
{
    char * path = NULL;
    int fildes, count;
    struct dirent dbuf[10];

    if (argc > 1)
        path = argv[1];

    if (path == NULL || !strcmp(path, ""))
        path = "./";

    fildes = open(path, O_DIRECTORY | O_RDONLY | O_SEARCH);
    if (fildes < 0) {
        printf("Open failed\n");
        return 1;
    }

    while ((count = getdents(fildes, (char *)dbuf, sizeof(dbuf))) > 0) {
        for (int i = 0; i < count; i++) {
            struct stat stat;
            char mode[12];

            fstatat(fildes, dbuf[i].d_name, &stat, 0);

            strmode(stat.st_mode, mode);
            printf("% 7u %s %u:%u %s\n",
                     (unsigned)dbuf[i].d_ino, mode,
                     (unsigned)stat.st_uid, (unsigned)stat.st_gid,
                     dbuf[i].d_name);
        }
    }
    printf("\n");

    close(fildes);
    return 0;
}
