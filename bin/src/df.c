/**
 *******************************************************************************
 * @file    df.c
 * @author  Olli Vanhoja
 * @brief   df.
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

#include <dirent.h>
#include <fcntl.h>
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

char * argv0;
struct {
    int k : 1;
    int P : 1;
} flags;

struct format_str {
    const char header[80];
    const char entry[80];
} format_str[] = {
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

static int fscan_fs(char * buf, size_t n, FILE * fp)
{
    int c;
    char * p = buf;
    const char * end = buf + n - 1;

    while (1) {
        c = fgetc(fp);
        if (c != EOF && c != ' ' && c != '\0' && p != end) {
            *p++ = (char)c;
            continue;
        }
        break;
    }
    *p = '\0';

    return p != buf;
}

static inline int open_root(dev_t dev)
{
    return syscall(SYSCALL_FS_OPEN_ROOT, &dev);
}

static inline int chdir_fd(int fd)
{
    struct _proc_chdir_args args = {
        .fd = fd,
        .name = ".",
        .name_len = 2,
        .atflags = AT_FDARG
    };

    return syscall(SYSCALL_PROC_CHDIR, &args);
}

static int rdev2path(char buf[256], dev_t rdev)
{
    DIR * dir;
    struct dirent * dp;
    int dfd, retval = -1;

    if ((dir = fdopendir((dfd = open("/dev", O_RDONLY)))) == NULL) {
        fprintf(stderr, "Cannot open /dev directory\n");
        exit(EX_OSFILE);
    }

    while ((dp = readdir(dir)) != NULL) {
        int ffd, err;
        struct stat st;

        if (dp->d_name[0] == '.')
            continue;

        if ((ffd = openat(dfd, dp->d_name, O_RDONLY)) == -1) {
            perror(dp->d_name);
            continue;
        }
        err = fstat(ffd, &st);
        close(ffd);
        if (err)
            continue;

        if (st.st_mode & (S_IFBLK | S_IFCHR) && st.st_rdev == rdev) {
            snprintf(buf, 256, "/dev/%s", dp->d_name);
            retval = 0;
            break;
        }
    }
    closedir(dir);

    return retval;
}

char cwd_buf[PATH_MAX];
static void print_df(const struct statvfs * restrict st, const char * fs)
{
    char * cwd;
    const unsigned k = (flags.k ? 1024 : 512);
    int blocks, used, avail, capacity;

    blocks = (st->f_blocks * st->f_frsize) / k;
    used = (st->f_blocks - st->f_bfree) * st->f_frsize / k;
    avail = st->f_bfree * st->f_frsize / k;
    capacity = 100 * (st->f_blocks - st->f_bfree) / st->f_blocks;
    cwd = getcwd(cwd_buf, sizeof(cwd_buf));

    /* TODO Resolve the device */
    if (flags.P) {
        printf(format_str[1].entry, fs, blocks, used, avail, capacity, cwd);
    } else {
        int iused = st->f_files - st->f_ffree;
        int piused = 100 * iused / st->f_files;

        printf(format_str[0].entry, fs, blocks, used, avail, capacity, iused,
               st->f_ffree, piused, cwd);
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
        case '?':
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
    } else {
        FILE * mounts;
        int major, minor, rdev_major, rdev_minor;
        char fs[14];

        if ((mounts = fopen("/proc/mounts", "r")) == NULL) {
            perror("Cannot open /proc/mounts");
            exit(EX_OSFILE);
        }

        while (fscan_fs(fs, sizeof(fs), mounts) &&
               fscanf(mounts, "(%d,%d) (%d,%d)\n",
                      &major, &minor, &rdev_major, &rdev_minor) != EOF) {
            int err, fd;
            struct statvfs st;

            if (fs[0] == '\0')
                break;

            fd = open_root(DEV_MMTODEV(major, minor));
            if (fd < 0) {
                perror("Cannot open a filesystem root");
                exit(EX_OSERR);
            }
            chdir_fd(fd);

            if (rdev_major >= 0 && rdev_minor >= 0) {
                rdev2path(fs, DEV_MMTODEV(rdev_major, rdev_minor));
            }

            err = fstatvfs(fd, &st);
            close(fd);
            if (err) {
                perror("Failed to stat a filesystem");
                continue;
            }
            print_df(&st, fs);
        }
        fclose(mounts);
    }

    return 0;
}
