/**
 *******************************************************************************
 * @file    fatfs.h
 * @author  Olli Vanhoja
 * @brief   FatFs public header.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup fs
 * @{
 */

/**
 * @addtogroup fatfs
 * @{
 */

#ifndef FATFS_H
#define FATFS_H

#include <libkern.h>
#include <fs/fs.h>
#include <fs/inpool.h>
#include "src/ff.h"

#define FATFS_FSNAME            "fatfs"

struct fatfs_inode {
    vnode_t in_vnode;   /*!< vnode for this inode. */
    char * in_fpath;    /*!< Full path to this node from the sb root. */
    atomic_t open_count;

    /**
     * file pointer or directory pointer, check in_vnode->vn_mode.
     */
    union {
    FF_FIL fp;
    FF_DIR dp;
    };
};

/**
 * FatFs superblock.
 */
struct fatfs_sb {
    struct fs_superblock sb;    /*!< Superblock node. */
    inpool_t inpool;            /*!< inode pool. */
    file_t ff_devfile;          /*!< Fs device. */
    FATFS ff_fs;                /*!< ff descriptor. */
};

/**
 * Get fatfs_sb of a generic superblock that belongs to fatfs.
 * @param _sb_  is a pointer to a superblock pointing some ramfs mount.
 * @return Returns a pointer to the ramfs_sb ob of the sb.
 */
#define get_ffsb_of_sb(_sb_) \
    (containerof(_sb_, struct fatfs_sb, sb))

/**
 * Get corresponding inode of given vnode.
 * @param vn    is a pointer to a vnode.
 * @return Returns a pointer to the inode.
 */
#define get_inode_of_vnode(vn) \
    (containerof(vn, struct fatfs_inode, in_vnode))

struct fatfs_sb ** fatfs_sb_arr;

ssize_t fatfs_read(file_t * file, struct uio * uio, size_t count);
ssize_t fatfs_write(file_t * file, struct uio * uio, size_t count);
int fatfs_create(vnode_t * dir, const char * name, mode_t mode,
                 vnode_t ** result);
int fatfs_mknod(vnode_t * dir, const char * name, int mode, void * specinfo,
                vnode_t ** result);
int fatfs_unlink(vnode_t * dir, const char * name);
int fatfs_mkdir(vnode_t * dir,  const char * name, mode_t mode);
int fatfs_rmdir(vnode_t * dir,  const char * name);
int fatfs_readdir(vnode_t * dir, struct dirent * d, off_t * off);
int fatfs_stat(vnode_t * vnode, struct stat * buf);
int fatfs_chmod(vnode_t * vnode, mode_t mode);
int fatfs_chflags(vnode_t * vnode, fflags_t flags);

#endif /* FATFS_H */

/**
 * @}
 */

/**
 * @}
 */
