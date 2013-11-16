/**
 *******************************************************************************
 * @file    fs.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system headers.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

/** @addtogroup fs
  * @{
  */

#pragma once
#ifndef FS_H
#define FS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <syscalldef.h>
#include <devtypes.h>

#define FS_FLAG_INIT       0x01 /*!< Filse system initialized. */
#define FS_FLAG_FAIL       0x08 /*!< File system has failed. */

/* Some macros for use with flags *********************************************/
/**
 * Test act_flags for FS_FLAG_INIT.
 * @param act_flags actual flag values.
 */
#define FS_TFLAG_INIT(act_flags)       ((act_flags & FS_FLAG_INIT) != 0)

/**
 * Test act_flags for FS_FLAG_FAIL.
 * @param act_flags actual flag values.
 */
#define FS_TFLAG_FAIL(act_flags)       ((act_flags & FS_FLAG_FAIL) != 0)

/**
 * Test act_flags for any of exp_flags.
 * @param act_flags actual flag values.
 * @param exp_flags expected flag values.
 */
#define FS_TFLAGS_ANYOF(act_flags, exp_flags) ((act_flags & exp_flags) != 0)

/**
 * Test act_flags for all of exp_flags.
 * @param act_flags actual flag values.
 * @param exp_flags expected flag values.
 */
#define FS_TFLAGS_ALLOF(act_flags, exp_flags) ((act_flags & exp_flags) == exp_flags)
/* End of macros **************************************************************/

typedef struct {
    size_t vnode_num;   /*!< vnode number. */
    dev_t dev;
    int refcount;
    size_t len;         /*!< Length of file. */
    size_t mutex;
    mode_t mode;        /*!< File type part of st_mode sys/stat.h */
    struct fs_superblock_t;
    struct vnode_ops * vnode_ops;
    /* TODO wait queue here */
} vnode_t;

/**
 * File descriptor.
 */
typedef struct file {
    int pos;        /*!< Seek pointer. */
    mode_t mode;    /*!< Access mode. */
    int refcount;   /*!< Reference count. */
    vnode_t * vnode;
} file_t;

/**
 * File system.
 */
typedef struct {
    char fsname[8];
    struct fs_superblock * (*mount)(vnode_t * vnode, char * mpoint, uint32_t mode);
    int (*umount)(struct fs_superblock * fs);
} fs_t;

/**
 * File system superblock.
 */
typedef struct fs_superblock {
    char fsname[8];
    uint32_t mode_flags;
    vnode_t * root; /*!< Root of this fs mount. */
    int (*lookup_vnode)(vnode_t * vnode, char * str);
    int (*lookup_file)(char * str, vnode_t * file);
    int (*delete_vnode)(vnode_t * vnode);
} fs_superblock_t;

/**
 * vnode operations struct.
 */
typedef struct file_ops {
    /* Normal file operations */
    int (*lock)(vnode_t * file);
    int (*release)(vnode_t * file);
    int (*write)(vnode_t * file, int offset, const void * buf, int count);
    int (*read)(vnode_t * file, int offset, void * buf, int count);
    //int (*mmap)(vnode_t * file, !mem area!);
    /* Directory file operations */
    int (*create)(vnode_t * dir, const char * name, int name_len, vnode_t ** result);
    int (*mknod)(vnode_t * dir, const char * name, int name_len, int mode, dev_t dev);
    int (*lookup)(vnode_t * dir, const char * name, int name_len, vnode_t ** result);
    int (*link)(vnode_t * oldvnode, vnode_t * dir, const char * name, int name_len);
    int (*unlink)(vnode_t * dir, const char * name, int name_len);
    int (*mkdir)(vnode_t * dir,  const char * name, int name_len);
    int (*rmdir) (vnode_t * dir,  const char * name, int name_len);
    //int (*readdir)(vnode_t * dir, int offset, struct dirent * d);
    /* Operations specified for any file type */
    int (*stat)(vnode_t * vnode, struct stat * buf);
} vnode_ops_t;

int fs_register(fs_t * fs);

/* Thread specific functions used mainly by Syscalls **************************/
uint32_t fs_syscall(uint32_t type, void * p);

#endif /* FS_H */

/**
  * @}
  */
