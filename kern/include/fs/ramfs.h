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

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <fs/fs.h>

#define RAMFS_FSNAME            "ramfs"

extern vnode_ops_t ramfs_vnode_ops;

/* fs ops */
int ramsfs_mount(const char * source, uint32_t mode,
                 const char * parm, int parm_len, struct fs_superblock ** sb);
int ramfs_umount(struct fs_superblock * fs_sb);
/* sb ops */
int ramfs_get_vnode(struct fs_superblock * sb, ino_t * vnode_num, vnode_t ** vnode);
int ramfs_delete_vnode(vnode_t * vnode);
/* vnode ops */
ssize_t ramfs_write(file_t * file, const void * buf, size_t count);
ssize_t ramfs_read(file_t * file, void * buf, size_t count);
int ramfs_create(vnode_t * dir, const char * name, mode_t mode,
                 vnode_t ** result);
int ramfs_mknod(vnode_t * dir, const char * name, int mode, void * specinfo,
                vnode_t ** result);
int ramfs_lookup(vnode_t * dir, const char * name, vnode_t ** result);
int ramfs_link(vnode_t * dir, vnode_t * vnode, const char * name);
int ramfs_unlink(vnode_t * dir, const char * name);
int ramfs_mkdir(vnode_t * dir,  const char * name, mode_t mode);
int ramfs_rmdir(vnode_t * dir,  const char * name);
int ramfs_readdir(vnode_t * dir, struct dirent * d, off_t * off);
int ramfs_stat(vnode_t * vnode, struct stat * buf);
int ramfs_chmod(vnode_t * vnode, mode_t mode);
int ramfs_chown(vnode_t * vnode, uid_t owner, gid_t group);

#endif /* FS_RAMFS_H */
