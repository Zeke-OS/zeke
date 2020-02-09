/**
 *******************************************************************************
 * @file    statvfs.h
 * @author  Olli Vanhoja
 * @brief   File system information.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
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

/**
 * @addtogroup libc
 * @{
 */

#ifndef STATVFS_H
#define STATVFS_H

#include <sys/types/_fsblkcnt_t.h>
#include <sys/types/_fsfilcnt_t.h>

struct statvfs {
    unsigned long   f_bsize;    /*!< File system block size. */
    unsigned long   f_frsize;   /*!< Fragment size. */
    fsblkcnt_t      f_blocks;   /*!< Size of fs in f_frsize units. */
    fsblkcnt_t      f_bfree;    /*!< Number of free blocks */
    fsblkcnt_t      f_bavail;   /*!< Number of free blocks available to
                                 *   non-privileged process. */
    fsfilcnt_t      f_files;    /*!< Total number of inodes. */
    fsfilcnt_t      f_ffree;    /*!< Total number of free inodes. */
    fsfilcnt_t      f_favail;   /*!< Total number of free inodes to
                                 *   non-privileged process. */
    unsigned long   f_fsid;     /*!< Filesystem ID. */
    unsigned long   f_flag;     /*!< Mount flags. */
    unsigned long   f_namemax;  /*!< Maximum filename length. */
    char            fsname[8];  /*!< File system name. */
};

/* These must be in sync with MNT_ macros defined in mount.h */
#define ST_RDONLY      0x0001 /*!< Read only. */
#define ST_SYNCHRONOUS 0x0002 /*!< Synchronous writes. */
#define ST_ASYNC       0x0040 /*!< Asynchronous writes. */
#define ST_NOEXEC      0x0004 /*!< No exec for the file system. */
#define ST_NOSUID      0x0008 /*!< Set uid bits not honored. */
#define ST_NOATIME     0x0100 /*!< Don't update file access times. */

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/** Arguments for SYSCALL_FS_STATFS */
struct _fs_statfs_args {
    int fd;
    const char * path;
    size_t path_len;
    struct statvfs * buf;
    unsigned flags;
};

/** Arguments for SYSCALL_FS_GETFSSTAT */
struct _fs_getfsstat_args {
    struct statvfs * buf;
    size_t bufsize;
    unsigned flags;
};
#endif

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
int fstatvfs(int fildes, struct statvfs * buf);
int fstatvfsat(int fildes, const char * path, struct statvfs * buf);
int statvfs(const char * restrict path, struct statvfs * restrict buf);

/**
 * Get list of all mounted file systems.
 * @param buf       is a pointer to the buffer; NULL if peeking the required buffer size.
 * @param bufsize   is the buffer size; 0 if peeking the required buffer size.
 * @param flags     No flags specified.
 * @returns Returns the size of stats written in bytes if successful;
 *          Otherwise returns -1 and sets errno.
 */
int getfsstat(struct statvfs * buf, long bufsize, int flags);
__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* STATVFS_H */

/**
 * @}
 */
