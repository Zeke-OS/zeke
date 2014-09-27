/**
 *******************************************************************************
 * @file    fz_fs.h
 * @author  Olli Vanhoja
 * @brief   FatFs public header.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef FATFS_H
#define FATFS_H

#include <libkern.h>
#include <fs/fs.h>
#include "src/ff.h"

#define FATFS_FSNAME            "fatfs"
#define FATFS_VDEV_MAJOR_ID     13

struct fatfs_inode {
    vnode_t in_vnode;   /*!< vnode for this inode. */
    char * in_fpath;    /*!< Full path to this node from the sb root. */

    /**
     * file pointer or directory pointer, check in_vnode->vn_mode.
     */
    union {
    FIL fp;
    DIR dp;
    };
};

/**
 * FatFs superblock.
 */
struct fatfs_sb {
    superblock_lnode_t sbn; /*!< Superblock node. */
    file_t ff_devfile;
    FATFS ff_fs;
    ino_t ff_ino;
};

/**
 * Get fatfs_sb of a generic superblock that belongs to fatfs.
 * @param sb    is a pointer to a superblock pointing some ramfs mount.
 * @return Returns a pointer to the ramfs_sb ob of the sb.
 */
#define get_ffsb_of_sb(sb) \
    (container_of(container_of(sb, superblock_lnode_t, sbl_sb), \
                  struct fatfs_sb, sbn))

/**
 * Get corresponding inode of given vnode.
 * @param vn    is a pointer to a vnode.
 * @return Returns a pointer to the inode.
 */
#define get_inode_of_vnode(vn) \
    (container_of(vn, struct fatfs_inode, in_vnode))

struct fatfs_sb ** fatfs_sb_arr;


ssize_t fatfs_write(file_t * file, const void * buf, size_t count);
ssize_t fatfs_read(file_t * file, void * buf, size_t count);
int fatfs_create(vnode_t * dir, const char * name, size_t name_len, mode_t mode,
                 vnode_t ** result);
int fatfs_mknod(vnode_t * dir, const char * name, size_t name_len, int mode,
                void * specinfo, vnode_t ** result);
int fatfs_link(vnode_t * dir, vnode_t * vnode, const char * name,
               size_t name_len);
int fatfs_unlink(vnode_t * dir, const char * name, size_t name_len);
int fatfs_mkdir(vnode_t * dir,  const char * name, size_t name_len,
                mode_t mode);
int fatfs_rmdir(vnode_t * dir,  const char * name, size_t name_len);
int fatfs_readdir(vnode_t * dir, struct dirent * d, off_t * off);
int fatfs_stat(vnode_t * vnode, struct stat * buf);
int fatfs_chmod(vnode_t * vnode, mode_t mode);
int fatfs_chown(vnode_t * vnode, uid_t owner, gid_t group);

#endif /* FATFS_H */
