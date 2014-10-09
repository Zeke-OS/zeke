/**
 *******************************************************************************
 * @file    tish_fs.c
 * @author  Olli Vanhoja
 * @brief   File system manipulation commands for tish/Zeke.
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
#include "tish.h"

static void cd(char ** args)
{
    char * arg = strtok_r(0, DELIMS, args);
    if (!arg)
        fprintf(stderr, "cd missing argument.\n");
    else
        chdir(arg);
}
TISH_CMD(cd, "cd");

static void ls(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);
    int fildes, count;
    struct dirent dbuf[10];

    if (path == 0 || !strcmp(path, ""))
        path = "./";

    fildes = open(path, O_DIRECTORY | O_RDONLY | O_SEARCH);
    if (fildes < 0) {
        printf("Open failed\n");
        return;
    }

    while ((count = getdents(fildes, (char *)dbuf, sizeof(dbuf))) > 0) {
        for (int i = 0; i < count; i++) {
            struct stat stat;

            fstatat(fildes, dbuf[i].d_name, &stat, 0);

            printf("%u %o %u:%u %s\n",
                     (uint32_t)dbuf[i].d_ino, (uint32_t)stat.st_mode,
                     (uint32_t)stat.st_uid, (uint32_t)stat.st_gid,
                     dbuf[i].d_name);
        }
    }
    printf("\n");

    close(fildes);
}
TISH_CMD(ls, "ls");

static void touch(char ** args)
{
    int fildes;
    char * path = strtok_r(0, DELIMS, args);

    fildes = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fildes < 0)
        return; /* err */

    close(fildes);
}
TISH_CMD(touch, "touch");

static void tish_mkdir(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);

    mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP);
}
TISH_CMD(tish_mkdir, "mkdir");

static void tish_rmdir(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);

    rmdir(path);
}
TISH_CMD(tish_rmdir, "rmdir");

static void tish_unlink(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);

    unlink(path);
}
TISH_CMD(tish_unlink, "unlink");

static void tish_cat(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);
    int fildes = open(path, O_RDONLY);
    char buf[80];
    ssize_t ret;

    if (fildes < 0)
        return; /* err */

    buf[sizeof(buf) - 1] = '\0';
    while (1) {
        ret = read(fildes, &buf, sizeof(buf) - 1);
        if (ret <= 0)
            break;
        puts(buf);
    }

    close(fildes);
}
TISH_CMD(tish_cat, "cat");
