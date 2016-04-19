/**
 *******************************************************************************
 * @file    ps.c
 * @author  Olli Vanhoja
 * @brief   ps.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

#define PROC_PATH   "/proc"
#define DEV_PATH    "/dev"

struct pstat {
    char name[16];
    pid_t pid;
    pid_t pgrp;
    pid_t sid;
    dev_t ctty; /* Controlling TTY */
    uid_t ruid;
    uid_t euid;
    uid_t suid;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;
    clock_t utime;
    clock_t stime;
};

struct ttydev {
    dev_t dev;
    char name[16];
};

static char pathbuf[PATH_MAX];
static struct ttydev ttydev[10];

static char * get_next_line(FILE * fp)
{
    static char line[MAX_INPUT];

    while (fgets(line, sizeof(line), fp)) {
        char * cp;
        int ch;

        cp = strchr(line, '\n');
        if (cp) {
            *cp = '\0';
            return line;
        }

        /* skip lines that are too long */
        while ((ch = fgetc(fp)) != '\n' && ch != EOF);
    }

    return NULL;
}

static int scan_proc(struct pstat * ps, FILE * fp)
{
    char * line;

    while ((line = get_next_line(fp))) {
        char * a;
        char * b;

        a = strsep(&line, ":");
        b = util_skipwhite(line);

        if (strcmp(a, "Name") == 0) {
            strlcpy(ps->name, b, sizeof(ps->name));
        } else if (strcmp(a, "Pid") == 0) {
            sscanf(b, "%u", &ps->pid);
        } else if (strcmp(a, "Pgrp") == 0) {
            sscanf(b, "%u", &ps->pgrp);
        } else if (strcmp(a, "Sid") == 0) {
            sscanf(b, "%u", &ps->sid);
        } else if (strcmp(a, "Ctty") == 0) {
            sscanf(b, "%u", &ps->ctty);
        } else if (strcmp(a, "User") == 0) {
            sscanf(b, "%d", &ps->utime);
        } else if (strcmp(a, "Sys") == 0) {
            sscanf(b, "%d", &ps->stime);
        }
        /* TODO UID & GID */
    }

    return 0;
}

void init_ttydev_arr(void)
{
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

int main(int argc, char * argv[], char * envp[])
{
    DIR * dirp;
    struct dirent * d;
    long clk_tck;

    clk_tck = sysconf(_SC_CLK_TCK);
    init_ttydev_arr();

    dirp = opendir(PROC_PATH);
    if (!dirp) {
        perror("Open failed");
        return EX_OSERR;
    }

    printf("  PID TTY          TIME CMD\n");
    while ((d = readdir(dirp))) {
        FILE * fp;
        struct pstat ps;
        clock_t sutime;

        if (!(d->d_type & DT_DIR) || d->d_name[0] == '.')
            continue;

        snprintf(pathbuf, sizeof(pathbuf), PROC_PATH "/%s/status",
                 d->d_name);
        fp = fopen(pathbuf, "r");
        if (!fp)
            continue;

        scan_proc(&ps, fp);

        sutime = (ps.utime + ps.stime) / clk_tck;
        printf("%5s %-6s   %02u:%02u:%02u %s\n",
               d->d_name,
               devttytostr(ps.ctty),
               sutime / 3600, (sutime % 3600) / 60, sutime % 60,
               ps.name);
        fclose(fp);
    }

    closedir(dirp);

    return 0;
}
