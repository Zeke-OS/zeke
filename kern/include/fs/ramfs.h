/**
 *******************************************************************************
 * @file    ramfs.h
 * @author  Olli Vanhoja
 * @brief   ramfs headers.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef FS_RAMFS_H
#define FS_RAMFS_H

#include <stddef.h>
#include <sys/types.h>

#define RAMFS_FSNAME            "ramfs"

struct fs;
struct fs_superblock;
struct vnode;
struct vnode_ops;

extern struct vnode_ops ramfs_vnode_ops;

/* fs ops */
int ramsfs_mount(const char * source, uint32_t mode,
                 const char * parm, int parm_len, struct fs_superblock ** sb);
int ramfs_umount(struct fs_superblock * fs_sb);
/* sb ops */
int ramfs_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                    struct vnode ** vnode);
int ramfs_delete_vnode(struct vnode * vnode);
/* vnode ops */
ssize_t ramfs_write(struct file * file, const void * buf, size_t count);
ssize_t ramfs_read(struct file * file, void * buf, size_t count);
int ramfs_create(struct vnode * dir, const char * name, mode_t mode,
                 struct vnode ** result);
int ramfs_mknod(struct vnode * dir, const char * name, int mode,
                void * specinfo, struct vnode ** result);
int ramfs_lookup(struct vnode * dir, const char * name, struct vnode ** result);
int ramfs_link(struct vnode * dir, struct vnode * vnode, const char * name);
int ramfs_unlink(struct vnode * dir, const char * name);
int ramfs_mkdir(struct vnode * dir,  const char * name, mode_t mode);
int ramfs_rmdir(struct vnode * dir,  const char * name);
int ramfs_readdir(struct vnode * dir, struct dirent * d, off_t * off);
int ramfs_stat(struct vnode * vnode, struct stat * buf);
int ramfs_chmod(struct vnode * vnode, mode_t mode);
int ramfs_chown(struct vnode * vnode, uid_t owner, gid_t group);

#endif /* FS_RAMFS_H */
