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

static void usage(void)
{
    fprintf(stderr, "usage: %s [-la] [dir]\n", argv0);

    exit(EX_USAGE);
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
            strlcpy(buf, dp->d_name, 256);
            retval = 0;
            break;
        }
    }
    closedir(dir);

    return retval;
}

char cwd_buf[PATH_MAX];
static void print_df(int fd, const char * fs)
{
    struct statvfs st;
    char * cwd;
    const unsigned k = (flags.k ? 1024 : 512);
    int blocks, used, avail, capacity;

    if (fstatvfs(fd, &st)) {
        perror("Failed to stat a filesystem");
        return;
    }

    blocks = (st.f_blocks * st.f_frsize) / k;
    used = (st.f_blocks - st.f_bfree) * st.f_frsize / k;
    avail = st.f_bfree * st.f_frsize / k;
    capacity = 100 * (st.f_blocks - st.f_bfree) / st.f_blocks;
    cwd = getcwd(cwd_buf, sizeof(cwd_buf));

    /* TODO Resolve the device */
    if (flags.P) {
        printf("%s %d %d %d %d%% %s\n",
               fs, blocks, used, avail, capacity, cwd);
    } else {
        int iused = st.f_files - st.f_ffree;
        int piused = 100 * iused / st.f_files;

        printf("%s %d %d %d %d%% %d %d %d%% %s\n",
               fs, blocks, used, avail, capacity, iused, st.f_ffree, piused,
               cwd);
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

    if (flags.P) {
        if (flags.k) {
            printf("Filesystem  1024-blocks  Used  Available  Capacity  Mounted on\n");
        } else {
            printf("Filesystem  512-blocks  Used  Available  Capacity  Mounted on\n");
        }
    } else {
        if (flags.k) {
            printf("Filesystem  1024-blocks  Used  Available  Capacity  iused  ifree  %iused  Mounted on\n");
        } else {
            printf("Filesystem  512-blocks  Used  Available  Capacity  iused  ifree  %iused  Mounted on\n");
        }
    }

    if (*argv) {
        /* TODO file arg support */
    } else {
        FILE * mounts;
        int major, minor, rdev_major, rdev_minor;
        char fs[256];

        if ((mounts = fopen("/proc/mounts", "r")) == NULL) {
            perror("Cannot open /proc/mounts");
            exit(EX_OSFILE);
        }

        while (!!(fs[0] = '-') && !(fs[1] = '\0') &&
               fscanf(mounts, "%s (%d,%d) (%d,%d)\n",
                      fs, &major, &minor, &rdev_major, &rdev_minor) > 0) {
            int fd;

            fd = open_root(DEV_MMTODEV(major, minor));
            chdir_fd(fd);
            if (fd < 0) {
                perror("Cannot open a filesystem root");
                exit(EX_OSERR);
            }

            if (rdev_major >= 0 && rdev_minor >= 0) {
                rdev2path(fs, DEV_MMTODEV(rdev_major, rdev_minor));
            }
            print_df(fd, fs);
            close(fd);
        }
        fclose(mounts);
    }

    return 0;
}
