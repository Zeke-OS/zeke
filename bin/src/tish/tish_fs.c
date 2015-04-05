/**
 *******************************************************************************
 * @file    tish_fs.c
 * @author  Olli Vanhoja
 * @brief   File system manipulation commands for tish/Zeke.
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
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "tish.h"

static int cd(char ** args)
{
    char * arg = strtok_r(0, DELIMS, args);

    if (!arg) {
        fprintf(stderr, "cd missing argument.\n");
        errno = EINVAL;
        return -1;
    }
    chdir(arg);

    return 0;
}
TISH_CMD(cd, "cd");

char cwd_buf[PATH_MAX];
static int pwd(char ** args)
{
    char * cwd;

    cwd = getcwd(cwd_buf, sizeof(cwd_buf));
    if (!cwd) {
        perror("Failed to get cwd");
        return -1;
    }
    printf("%s\n", cwd);

    return 0;
}
TISH_CMD(pwd, "pwd");

static int touch(char ** args)
{
    int fildes;
    char * path = strtok_r(0, DELIMS, args);

    fildes = creat(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fildes < 0)
        return -1; /* err */

    return close(fildes);
}
TISH_CMD(touch, "touch");

static int tish_mkdir(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);

    return mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP);
}
TISH_CMD(tish_mkdir, "mkdir");

static int tish_rmdir(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);

    return rmdir(path);
}
TISH_CMD(tish_rmdir, "rmdir");

static int tish_unlink(char ** args)
{
    char * path = strtok_r(0, DELIMS, args);

    return unlink(path);
}
TISH_CMD(tish_unlink, "unlink");
