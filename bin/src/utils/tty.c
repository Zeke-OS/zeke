/**
 *******************************************************************************
 * @file    tty.c
 * @author  Olli Vanhoja
 * @brief   TTY id to string.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2016, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEV_PATH    "/dev"

struct ttydev {
    dev_t dev;
    char name[16];
};

static struct ttydev ttydev[10];

void init_ttydev_arr(void)
{
    static char pathbuf[PATH_MAX];
    DIR * dirp;
    struct dirent * d;
    size_t i = 0;

    dirp = opendir(DEV_PATH);
    if (!dirp) {
        perror("Getting TTY list failed");
        return;
    }

    while ((d = readdir(dirp))) {
        int fildes, istty;
        struct stat statbuf;

        if (d->d_name[0] == '.' || !(d->d_type & DT_CHR))
            continue;

        snprintf(pathbuf, sizeof(pathbuf), DEV_PATH "/%s", d->d_name);

        fildes = open(pathbuf, O_RDONLY | O_NOCTTY);
        if (fildes == -1) {
            perror(pathbuf);
            continue;
        }

        istty = isatty(fildes);
        if (fstat(fildes, &statbuf)) {
            perror(pathbuf);
            continue;
        }

        close(fildes);

        if (!istty)
            continue;

        if (i >= num_elem(ttydev)) {
            fprintf(stderr, "Out of slots for TTYs");
            goto out;
        }
        ttydev[i].dev = statbuf.st_rdev;
        strlcpy(ttydev[i].name, d->d_name, sizeof(ttydev[0].name));
        i++;
    }

out:
    closedir(dirp);
}

char * devttytostr(dev_t tty)
{
    if (DEV_MAJOR(tty) == 0)
        goto out;

    for (size_t i = 0; i < num_elem(ttydev); i++) {
        if (ttydev[i].dev == tty) {
            return ttydev[i].name;
        }
    }

out:
    return "?";
}
