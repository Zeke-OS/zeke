/**
 *******************************************************************************
 * @file    devtypes.h
 * @author  Olli Vanhoja
 * @brief   Device driver types header file.
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

/** @addtogroup Library_Functions
  * @{
  */

#pragma once
#ifndef STAT_H
#define STAT_H

#include <stdint.h>
#include <sys/types.h>

struct stat {
    dev_t     st_dev;       /*!< ID of device containing file. */
    ino_t     st_ino;       /*!< File serial number */
    mode_t    st_mode;      /*!< Mode of file. */
    nlink_t   st_nlink;     /*!< Number of links to the file */
    uid_t     st_uid;       /*!< User ID of file. */
    gid_t     st_gid;       /*!< Group ID of file. */
    dev_t     st_rdev;      /*!< Device ID (if file is character or block special). */
    off_t     st_size;      /*!< File size in bytes (if file is a regular file). */
    time_t    st_atime;     /*!< Time of last access. */
    time_t    st_mtime;     /*!< Time of last data modification. */
    time_t    st_ctime;     /*!< Time of last status change. */
    blksize_t st_blksize;   /*!< A filesystem-specific preferred I/O block size
                                for this object.  In some filesystem types, this
                                may vary from file to file. */
    blkcnt_t  st_blocks;  /*!< Number of blocks allocated for this object. */
};

/*
int chmod(const char *, mode_t);
int fchmod(int, mode_t);
int fstat(int, struct stat *);
int lstat(const char *restrict, struct stat *restrict);
int mkdir(const char *, mode_t);
int mkfifo(const char *, mode_t);
int mknod(const char *, mode_t, dev_t);
int stat(const char *restrict, struct stat *restrict);
mode_t umask(mode_t);
*/

/* Extensions to POSIX */

int mount(const char * mount_point, const char * fsname, uint32_t mode,
        int parm_len, char * parm);

#endif /* STAT_H */

/**
  * @}
  */
