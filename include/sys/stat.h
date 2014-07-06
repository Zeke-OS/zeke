/**
 *******************************************************************************
 * @file    stat.h
 * @author  Olli Vanhoja
 * @brief   Data returned by the stat() function.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup libc
 * @{
 */

#pragma once
#ifndef STAT_H
#define STAT_H

#include <stdint.h>
#include <sys/types.h>

#ifndef TIME_H
    #error <time.h> shall be included.
#endif

struct stat {
    dev_t     st_dev;       /*!< ID of device containing file. */
    ino_t     st_ino;       /*!< File serial number */
    mode_t    st_mode;      /*!< Mode of file. */
    nlink_t   st_nlink;     /*!< Number of links to the file */
    uid_t     st_uid;       /*!< User ID of file. */
    gid_t     st_gid;       /*!< Group ID of file. */
    dev_t     st_rdev;      /*!< Device ID (if file is character or block special). */
    off_t     st_size;      /*!< File size in bytes (if file is a regular file). */
    struct timespec st_atime;     /*!< Time of last access. */
    struct timespec st_mtime;     /*!< Time of last data modification. */
    struct timespec st_ctime;     /*!< Time of last status change. */
    blksize_t st_blksize;   /*!< A filesystem-specific preferred I/O block size
                                for this object.  In some filesystem types, this
                                may vary from file to file. */
    blkcnt_t  st_blocks;  /*!< Number of blocks allocated for this object. */
};

/* Symbolic names for the values of st_mode
 * -----------------------------------------
 * File type bits: */
#define S_IFMT      0x0170000 /*!< Bit mask for the file type bit fields. */
#define S_IFBLK     0x0060000 /*!< Block device (special). */
#define S_IFCHR     0x0020000 /*!< Character device (special). */
#define S_IFIFO     0x0010000 /*!< FIFO special. */
#define S_IFREG     0x0100000 /*!< Regular file. */
#define S_IFDIR     0x0040000 /*!< Directory. */
#define S_IFLNK     0x0120000 /*!< Symbolic link. */
#define S_IFSOCK    0x0140000 /*!< Socked. */
/* File mode bits: */
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR) /*!< Mask for file owner permissions. */
#define S_IRUSR 0x0400 /*!< Owner has read permission. */
#define S_IWUSR 0x0200 /*!< Owner has write permission. */
#define S_IXUSR 0x0100 /*!< Owner has execute permission. */
#define S_IRWXG (S_IRGRP | S_IWGRP | S_IXGRP) /*!< Mask for group permissions. */
#define S_IRGRP 0x0040 /*!< Group has read permission. */
#define S_IWGRP 0x0020 /*!< Group has write permission. */
#define S_IXGRP 0x0010 /*!< Group has execute permission. */
#define S_IRWXO (S_IROTH | S_IWOTH | S_IXOTH) /*!< Mask for permissions for others. */
#define S_IROTH 0x0004 /*!< Others have read permission. */
#define S_IWOTH 0x0002 /*!< Others have write permission. */
#define S_IXOTH 0x0001 /*!< Others have execute permission. */
#define S_ISUID 0x4000 /*!< Set-user-ID bit. */
#define S_ISGID 0x2000 /*!< Set-group-ID bit. */
#define S_ISVTX 0x1000 /*!< On directories, restricted deletion flag. */

/** Test for a block special file. */
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
/** Test for a character special file. */
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
/** Test for a directory. */
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
/** Test for a pipe or FIFO special file. */
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
/** Test for a regular file. */
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
/** Test for a symbolic link. */
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
/** Test for a socked. */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
/*
int chmod(const char *, mode_t);
int fchmod(int, mode_t);
*/
int fstat(int fildes, struct stat * buf);
int fstatat(int fd, const char * restrict path,
            struct stat * restrict buf, int flag);
int lstat(const char * restrict path, struct stat * restrict buf);
int stat(const char * restrict path, struct stat * restrict buf);
/*
int mkdir(const char *, mode_t);
int mkfifo(const char *, mode_t);
int mknod(const char *, mode_t, dev_t);
mode_t umask(mode_t);
*/

/* Extensions to POSIX */

int mount(const char * mount_point, const char * fsname, uint32_t mode,
        int parm_len, char * parm);
__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* STAT_H */

/**
 * @}
 */
