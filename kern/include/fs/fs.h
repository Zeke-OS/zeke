/**
 *******************************************************************************
 * @file    fs.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system headers.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#define FS_FLAG_INIT        0x01 /*!< File system initialized. */
#define FS_FLAG_FAIL        0x08 /*!< File system has failed. */

#define FS_FILENAME_MAX     255 /*!< Maximum file name length. */
#define PATH_DELIMS "/"

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

typedef struct vnode {
    ino_t vn_num;       /*!< vnode number. */
    int vn_refcount;
    off_t vn_len;       /*!< Length of file. */
    size_t vn_mutex;
    mode_t vn_mode;     /*!< File type part of st_mode sys/stat.h */
    dev_t vn_devid;     /*!< Dev identifier in a case of cdev or bdev. */
    struct fs_superblock * sb; /*!< Pointer to the super block of this vnode. */
    struct vnode_ops * vnode_ops;
    /* TODO wait queue here */
} vnode_t;

/**
 * File descriptor.
 */
typedef struct file {
    off_t seek_pos; /*!< Seek pointer. */
    mode_t mode;    /*!< Access mode. */
    int refcount;   /*!< Reference count. */
    vnode_t * vnode;
} file_t;

/**
 * Files that a process has open.
 */
typedef struct files_struct {
        int count;
        struct file * fd[0]; /*!< Open files.
                              *   Thre should be at least following files:
                              *   [0] = stdin
                              *   [1] = stdout
                              *   [2] = stderr
                              */
} files_t;
/**
 * Size of files struct in bytes.
 * @param n is a file count.
 */
#define SIZEOF_FILES(n) (sizeof(files_t) + (n) * sizeof(file_t *))

/**
 * File system.
 */
typedef struct fs {
    char fsname[8];

    struct fs_superblock * (*mount)(const char * mpoint, uint32_t mode,
            int parm_len, char * parm);
    int (*umount)(struct fs_superblock * fs_sb);
    struct superblock_lnode * sbl_head; /*!< List of all mounts. */
} fs_t;

/**
 * File system superblock.
 */
typedef struct fs_superblock {
    fs_t * fs;
    dev_t dev;
    uint32_t mode_flags; /*!< Mount mode flags */
    vnode_t * root; /*!< Root of this fs mount. */
    char * mtpt_path; /*!< Mount point path */

    /**
     * Get the vnode struct linked to a vnode number.
     * @param[in]  sb is the superblock.
     * @param[in]  vnode_num is the vnode number.
     * @param[out] vnode is a pointer to the vnode.
     * @return Returns 0 if no error; Otherwise value other than zero.
     */
    int (*get_vnode)(struct fs_superblock * sb, ino_t * vnode_num, vnode_t ** vnode);

    /**
     * Delete a vnode reference.
     * Deletes a reference to a vnode and destroys the inode corresponding to the
     * inode if there is no more links and references to it.
     * @param[in] vnode is the vnode.
     * @return Returns 0 if no error; Otherwise value other than zero.
     */
    int (*delete_vnode)(vnode_t * vnode);
} fs_superblock_t;

/**
 * vnode operations struct.
 */
typedef struct vnode_ops {
    /* Normal file operations
     * ---------------------- */
    int (*lock)(vnode_t * file);
    int (*release)(vnode_t * file);
    size_t (*write)(vnode_t * file, const off_t * offset,
            const void * buf, size_t count);
    size_t (*read)(vnode_t * file, const off_t * offset,
            void * buf, size_t count);
    //int (*mmap)(vnode_t * file, !mem area!);
    /* Directory file operations
     * ------------------------- */
    int (*create)(vnode_t * dir, const char * name, size_t name_len,
            vnode_t ** result);
    int (*mknod)(vnode_t * dir, const char * name, size_t name_len, int mode,
            dev_t dev);
    int (*lookup)(vnode_t * dir, const char * name, size_t name_len,
            vnode_t ** result);
    int (*link)(vnode_t * dir, vnode_t * vnode, const char * name,
            size_t name_len);
    int (*unlink)(vnode_t * dir, const char * name, size_t name_len);
    int (*mkdir)(vnode_t * dir,  const char * name, size_t name_len);
    int (*rmdir)(vnode_t * dir,  const char * name, size_t name_len);
    int (*readdir)(vnode_t * dir, struct dirent * d);
    /* Operations specified for any file type */
    int (*stat)(vnode_t * vnode, struct stat * buf);
} vnode_ops_t;


/**
 * Superblock list type.
 */
typedef struct superblock_lnode {
    fs_superblock_t sbl_sb; /*!< Superblock struct. */
    struct superblock_lnode * next; /*!< Pointer to the next super block. */
} superblock_lnode_t;

/**
 * fs list type.
 */
typedef struct fsl_node {
    fs_t * fs; /*!< Pointer to the file system struct. */
    struct fsl_node * next; /*!< Pointer to the next fs list node. */
} fsl_node_t;

/**
 * Sperblock iterator.
 */
typedef struct sb_iterator {
    fsl_node_t * curr_fs; /*!< Current fs list node. */
    superblock_lnode_t * curr_sb; /*!< Current superblock of curr_fs. */
} sb_iterator_t;

/* VFS Function Prototypes */

/**
 * Register a new file system driver.
 * @param fs file system control struct.
 */
int fs_register(fs_t * fs);

/**
 * Initialize a file system superblock iterator.
 * Iterator is used to iterate over all superblocks of mounted file systems.
 * @param it is an untilitialized superblock iterator struct.
 */
void fs_init_sb_iterator(sb_iterator_t * it);

/**
 * Iterate over superblocks of mounted file systems.
 * @param it is the iterator struct.
 * @return The next superblock or 0.
 */
fs_superblock_t * fs_next_sb(sb_iterator_t * it);

/**
 * Get next free pseudo fs minor code.
 */
unsigned int fs_get_pfs_minor(void);

#endif /* FS_H */

/**
  * @}
  */
