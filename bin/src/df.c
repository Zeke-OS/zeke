/**
 *******************************************************************************
 * @file    df.c
 * @author  Olli Vanhoja
 * @brief   df.
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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <syscall.h>
#include <sysexits.h>

#define __SYSCALL_DEFS__
#include <unistd.h>

struct format_str {
    const char header[80];
    const char entry[80];
};

char * argv0;
struct {
    unsigned int k : 1;
    unsigned int P : 1;
} flags;

struct format_str format_str[] = {
    { /* Default */
        .header = "%-14s %10s %10s %10s %8s %10s %10s %6s %s\n",
        .entry = "%-14s %10d %10d %10d %7d%% %10d %10d %5d%% %s\n",
    },
    { /* P */
        .header = "%-14s %10s %10s %10s %8s %s\n",
        .entry = "%-14s %10d %10d %10d %7d%% %s\n",
    },
};

static void usage(void)
{
    fprintf(stderr, "usage: %s [-kP] <file>\n", argv0);

    exit(EX_USAGE);
}

char cwd_buf[PATH_MAX];
static void print_df(const struct statvfs * restrict st)
{
    const unsigned k = (flags.k ? 1024 : 512);
    const int blocks = (st->f_blocks * st->f_frsize) / k;
    const int used = (st->f_blocks - st->f_bfree) * st->f_frsize / k;
    const int avail = st->f_bfree * st->f_frsize / k;
    const int capacity = 100 * (st->f_blocks - st->f_bfree) / st->f_blocks;
    char * cwd = getcwd(cwd_buf, sizeof(cwd_buf));

    if (flags.P) {
        printf(format_str[1].entry,
               st->fsname, blocks, used, avail, capacity, cwd);
    } else {
        int iused = st->f_files - st->f_ffree;
        int piused = 100 * iused / st->f_files;

        printf(format_str[0].entry,
               st->fsname, blocks, used, avail, capacity, iused, st->f_ffree,
               piused, cwd);
    }
}

int main(int argc, char * argv[], char * envp[])
{
    int ch;

    argv0 = argv[0];

    while ((ch = getopt(argc, argv, "kP")) != EOF) {
        switch (ch) {
        case 'k':
            flags.k = 1;
            break;
        case 'P':
            flags.P = 1;
            break;
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    const char * blocks = flags.k ? "1024-blocks" : "512-blocks";
    if (flags.P) {
        printf(format_str[1].header, "Filesystem", blocks, "Used", "Available",
               "Capacity", "Mounted on");
    } else {
        printf(format_str[0].header, "Filesystem", blocks, "Used", "Available",
               "Capacity", "iused", "ifree", "%iused", "Mounted on");
    }

    if (*argv) {
        /* TODO file arg support */
#if 0
        err = fstatvfs(fd, &st);
#endif
    } else {
        int size = 0;
        struct statvfs * st = NULL;
        struct statvfs * end;

        do {
retry:
            size = getfsstat(st, size, 0);
            if (size > 0 && !st) {
                st = malloc(size);
                goto retry;
            } else if (size == -1) {
                size = 0;
                free(st);
                st = NULL;
            } else if (size == 0) {
                break;
            }
        } while (!st);

        end = st + size / sizeof(struct statvfs);
        while (st != end) {
            print_df(st);
            st++;
        }
    }

    return 0;
}
