/**
 *******************************************************************************
 * @file    fs.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system headers.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#pragma once
#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <machine/atomic.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <klocks.h>

#ifndef KERNEL_CONFIG_H
#error Needs autoconf
#endif

#define FS_FLAG_INIT    0x01 /*!< File system initialized. */
#define FS_FLAG_FAIL    0x08 /*!< File system has failed. */

#define PATH_DELIMS     "/"

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

/* Types for buffer pointer storage object in vnode. */
SPLAY_HEAD(bufhd_splay, buf);
struct bufhd {
    struct bufhd_splay sroot;
};

typedef struct vnode {
    ino_t vn_num;               /*!< vnode number. */
    unsigned vn_hash;           /*!< Hash for using vfs hashing. */
    atomic_t vn_refcount;

    /*!< Pointer to the vnode in mounted file system. If no fs is mounted on
     *   this vnode then this is a self pointing pointer. */
    struct vnode * vn_mountpoint;
    struct vnode * vn_prev_mountpoint;

    off_t vn_len;               /*!< Length of file, usually in bytes. */
    mode_t vn_mode;             /*!< File type part of st_mode sys/stat.h */
    void * vn_specinfo;         /*!< Pointer to an additional information
                                 * required by the ops. */

    /**
     * Pointer to a buffer pointer storage object.
     * vn_bpo represents a set of buffers belonging to the same vnode where
     * different buffers cover different non-overlapping ranges of data
     * within the vnode.
     */
    struct bufhd vn_bpo;

    /**
     * Pointer to the super block of this vnode.
     * Superblock is representing the actual file system mount.
     */
    struct fs_superblock * sb;

    /**
     * vnode operations.
     */
    struct vnode_ops * vnode_ops;

    /**
     * inpool:      Used for internal lists in inpool.
     */
    TAILQ_ENTRY(vnode) vn_inqueue;

#ifdef configVFS_HASH
    /**
     * vfs_hash:    (mount + inode) -> vnode hash. The hash value itself is
     *              grouped with other int fields, to avoid padding.
     */
    LIST_ENTRY(vnode) vn_hashlist;
#endif

    mtx_t vn_lock;
} vnode_t;
#define VN_LOCK_MODES (MTX_TYPE_TICKET | MTX_TYPE_SLEEP)
/* Test macros for vnodes */
#define VN_IS_FSROOT(vn) ((vn)->sb->root == (vn))
/* Op macros for vnodes */
#define VN_LOCK(vn) (mtx_lock(&(vn)->vn_lock))
#define VN_TRYLOCK(vn) (mtx_trylock(&(vn)->vn_lock))
#define VN_UNLOCK(vn) (mtx_unlock(&(vn)->vn_lock))
/*
 * Token indicating no attribute value yet assigned.
 */
#define VNOVAL  (-1)

/**
 * File descriptor.
 */
typedef struct file {
    off_t seek_pos;     /*!< Seek pointer. */
    int fdflags;        /*!< File descriptor flags. */
    int oflags;         /*!< File status flags. */
    int refcount;       /*!< Reference count. */
    vnode_t * vnode;
    void * stream;      /*!< Pointer to a special file stream data or info. */
    mtx_t lock;
} file_t;

/**
 * Files that a process has open.
 */
typedef struct files_struct {
        int count;
        mode_t umask;        /*!< File mode creation mask of the process. */
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

    int (*mount)(const char * source, uint32_t mode,
                 const char * parm, int parm_len, struct fs_superblock ** sb);
    int (*umount)(struct fs_superblock * fs_sb);
    struct superblock_lnode * sbl_head; /*!< List of all mounts. */
} fs_t;

/**
 * File system superblock.
 */
typedef struct fs_superblock {
    fs_t * fs;
    dev_t vdev_id;          /*!< Virtual dev_id. */
#ifdef configVFS_HASH
    unsigned sb_hashseed;   /*!< Seed for using vfs hashing */
#endif
    uint32_t mode_flags;    /*!< Mount mode flags */
    vnode_t * root;         /*!< Root of this fs mount. */
    vnode_t * mountpoint;   /*!< Mount point where this sb is mounted on.
                             *   (only vfs should touch this) */
    vnode_t * sb_dev;       /*!< Device for the file system. */

    /**
     * Get the vnode struct linked to a vnode number.
     * @note    This is an optional function and file systems are not required
     *          to implement this.
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
    /* Operation for open files
     * ------------------------ */
    int (*lock)(file_t * file);
    int (*release)(file_t * file);
    /**
     * Write transfers bytes from buf into file.
     * Writing is begin from offset and ended at offset + count. buf must
     * therefore contain at least count bytes. If offset is past end of the
     * current file the file will be extended; If offset is smaller than file
     * length, the existing data will be overwriten.
     * @param file      is a file stored in the file system.
     * @param buf       is a buffer where bytes are read from.
     * @param count     is the number of bytes buf contains.
     * @return  Returns the number of bytes written; Otherwise a negative errno
     *          is returned.
     */
    ssize_t (*write)(file_t * file, const void * buf, size_t count);
    /**
     * Read transfers bytes from file into buf.
     * @param file      is a file stored in the file system.
     * @param buf       is a buffer bytes are written to.
     * @param count     is the number of bytes to be read.
     * @return  Returns the number of bytes read; Otherwise a negative errno
     *          is returned.
     */
    ssize_t (*read)(file_t * file, void * buf, size_t count);
    /**
     * IO Control.
     * Only defined for devices and shall be set NULL if not supported.
     * @param file      is the open file accessed.
     * @param request   is the request number.
     * @param arg       is a pointer to the argument (struct).
     * @param ar_len    is the length of arg.
     * @return          0 ig succeed; Otherwise a negative errno is returned.
     */
    int (*ioctl)(file_t * file, unsigned request, void * arg, size_t arg_len);
    //int (*mmap)(vnode_t * file, !mem area!);
    /* Directory file operations
     * ------------------------- */
    /**
     * Create a new vnode with S_IFREG and a hard link with specified name for it
     * created in dir.
     * @param dir       is the directory vnode which is used to store the hard link
     *                  created.
     * @param name      is the name of the hard link.
     * @param[out] result is a pointer to the resulting vnode.
     * @return Zero in case of operation succeed; Otherwise value other than zero.
     */
    int (*create)(vnode_t * dir, const char * name, mode_t mode,
            vnode_t ** result);
    /**
     * Create a special vnode.
     * @note ops must be set manually after creation of a vnode.
     * @param specinfo  is a pointer to the special info struct.
     * @param mode      is the mode of the new file.
     */
    int (*mknod)(vnode_t * dir, const char * name, int mode, void * specinfo,
            vnode_t ** result);
    /**
     * Lookup for a hard linked vnode in a directory vnode.
     * @param dir       is a directory in the file system.
     * @param name      is a filename.
     * @param[out] result is the result of lookup.
     * @return Returns 0 if a vnode was found;
     *         If vnode is same as dir (root dir) -EDOM shall be returned;
     *         Otherwise value other than zero.
     */
    int (*lookup)(vnode_t * dir, const char * name, vnode_t ** result);
    /**
     * Create a hard link.
     * Link vnode into dir with the specified name.
     * @param dir       is the directory where entry will be created.
     * @param vnode     is a vnode where the link will point.
     * @param name      is the name of the hard link.
     * @return Returns 0 if creating a link succeeded; Otherwise value other than
     *         zero.
     */
    int (*link)(vnode_t * dir, vnode_t * vnode, const char * name);
    /**
     * Unlink a hard link.
     * Unlink a hard link in the directory specified.
     */
    int (*unlink)(vnode_t * dir, const char * name);
    /**
     * Create a directory called name in dir.
     * Implementation of mkdir() shall also set uid and gid of the new directory
     * if the underlying filesystem supports that feature.
     * @param dir       is a directory in the file system.
     * @param name      is the name of the new directory.
     * @param mode      is the file mode of the new directory.
     * @return Zero in case of operation succeed; Otherwise value other than zero.
     */
    int (*mkdir)(vnode_t * dir,  const char * name, mode_t mode);
    int (*rmdir)(vnode_t * dir,  const char * name);
    /**
     * Reads one directory entry from the dir into the struct dirent.
     * @param dir       is a directory open in the file system.
     * @param d         is a directory entry struct.
     * @param off       is the offset into the directory.
     * @return  Zero in case of operation succeed;
     *          -ENOTDIR if dir is not a directory;
     *          -ESPIPE if end of dir.
     */
    int (*readdir)(vnode_t * dir, struct dirent * d, off_t * off);

    /* Operations specified for any file type */
    /**
     * Get file status.
     */
    int (*stat)(vnode_t * vnode, struct stat * buf);

    int (*chmod)(vnode_t * vnode, mode_t mode);
    int (*chown)(vnode_t * vnode, uid_t owner, gid_t group);
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


/**
 * Generate insert_suberblock function.
 * Insert a sb_type struct to the end of sb mount linked list.
 * @param sb_type is a pointer to a file system superblock.
 */
#define GENERATE_INSERT_SB(sb_type, fs)                 \
static void insert_superblock(struct sb_type * sb)      \
{                                                       \
    superblock_lnode_t * curr = (fs).sbl_head;          \
                                                        \
    /* Add as a first sb if no other mounts yet */      \
    if (curr == 0) {                                    \
        (fs).sbl_head = &(sb->sbn);                     \
    } else {                                            \
        /* else find the last sb on the linked list. */ \
        while (curr->next != 0) {                       \
            curr = curr->next;                          \
        }                                               \
        curr->next = &(sb->sbn);                        \
    }                                                   \
}


/* VFS Function Prototypes */

/**
 * Register a new file system driver.
 * @param fs file system control struct.
 */
int fs_register(fs_t * fs);

/**
 * Lookup for vnode by path.
 * @param[out]  result  is the target where vnode struct is stored.
 * @param[in]   root    is the vnode where search is started from.
 * @param       str     is the path.
 * @param       oflags  Tested flags are: O_DIRECTORY, O_NOFOLLOW
 * @return  Returns zero if vnode was found;
 *          error code -errno in case of an error.
 */
int lookup_vnode(vnode_t ** result, vnode_t * root, const char * str, int oflags);

/**
 * Walks the file system for a process and tries to locate and lock vnode
 * corresponding to a given path.
 * @param fd        is the optional starting point for relative search.
 * @param atflags   if this is set to AT_FDARG then fd is used;
 *                  AT_FDCWD is implicit rule for this function.
 *                  AT_SYMLINK_NOFOLLOW is optional and AT_SYMLINK_FOLLOW is
 *                  implicit.
 */
int fs_namei_proc(vnode_t ** result, int fd, const char * path, int atflags);

/**
 * Mount file system.
 * @param target    is the target mount point directory.
 * @param vonde_dev is a vnode of a device or other mountable file system
 *                  device.
 * @param fsname    is the name of the file system type to mount. This
 *                  argument is optional and if left out fs_mount() tries
 *                  to determine the file system type from existing information.
 */
int fs_mount(vnode_t * target, const char * source, const char * fsname,
        uint32_t flags, const char * parm, int parm_len);

/**
 * Find registered file system by name.
 * @param fsname is the name of the file system.
 * @return The file system structure.
 */
fs_t * fs_by_name(const char * fsname);

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
 * Get next free pseudo fs minor number.
 */
unsigned int fs_get_pfs_minor(void);

int chkperm(struct stat * stat, uid_t uid, gid_t gid, int amode);

/**
 * Check file permissions against oflag(s).
 * This function accepts both oflags and amodes, except F_OK.
 * @param stat      is a pointer to stat struct of the file.
 * @param oflags    specifies the verified oflags or amodes.
 * @return  Returns negative errno in case of error or improper permissions;
 *          Otherwise zero.
 */
int chkperm_cproc(struct stat * stat, int oflags);

int chkperm_vnode_cproc(vnode_t * vnode, int oflags);
int chkperm_vnode(vnode_t * vnode, uid_t euid, gid_t egid, int oflags);

/**
 * Set parameters of a file descriptor.
 * @param vnode     is a vnode.
 * @param oflags    specifies the opening flags.
 * @return -errno.
 */
int fs_fildes_set(file_t * fildes, vnode_t * vnode, int oflags);

/**
 * Create a new file descriptor to the first empty spot.
 */
int fs_fildes_create_cproc(vnode_t * vnode, int oflags);

/**
 * Get next free file descriptor for the current process.
 * @param new_file  is the file to be stored in the next free position from
 *                  start.
 * @param start     is the start offset.
 */
int fs_fildes_cproc_next(file_t * new_file, int start);

/**
 * Increment or decrement a file descriptor reference count and free the
 * descriptor if there is no more refereces to it.
 * @param files     is the files struct where fd is searched for.
 * @param fd        is the file descriptor to be updated.
 * @param count     is the value to be added to the refcount.
 */
file_t * fs_fildes_ref(files_t * files, int fd, int count);

/**
 * Close file open for curproc.
 * @param fildes    is the file descriptor number.
 * @return  0 if file was closed;
 *          Otherwise a negative value representing errno.
 */
int fs_fildes_close_cproc(int fildes);

/**
 * Read or write to a open file of the current process.
 * @param fildes    is the file descriptor nuber.
 * @param buf       is the buffer.
 * @param nbytes    is the amount of bytes to be read/writted.
 * @param oper      is the file operation, either O_RDONLY or O_WRONLY.
 * @return  Return the number of bytes actually read/written from/to the file
 *          associated with fildes; Otherwise a negative value representing
 *          errno.
 */
ssize_t fs_readwrite_cproc(int fildes, void * buf, size_t nbyte, int oper);

/**
 * Create a new file by using fs specific create() function.
 * File will be created relative to the attributes of a current process.
 * @param[in]   path    is a path to the new file.
 * @param[in]   mode    selects the file mode.
 * @param[out]  result  is the new vnode created.
 * @return      0 if succeed; Negative error code representing errno in case of
 *              failure.
 */
int fs_creat_cproc(const char * path, mode_t mode, vnode_t ** result);

/**
 * Create a new link to existing vnode.
 */
int fs_link_curproc(const char * path1, size_t path1_len,
    const char * path2, size_t path2_len);

/**
 * Remove link to a file.
 */
int fs_unlink_curproc(int fd, const char * path, size_t path_len, int atflags);

/**
 * Remove link to a file relative to fd.
 */
int fs_unlinkat_curproc(int fd, const char * path, int flag);

/**
 * Create a new directory relative to the current process.
 */
int fs_mkdir_curproc(const char * pathname, mode_t mode);

int fs_rmdir_curproc(const char * pathname);

int fs_chmod_curproc(int fildes, mode_t mode);
int fs_chown_curproc(int fildes, uid_t owner, gid_t group);

vnode_t * fs_create_pseudofs_root(const char * fsname, int majornum);

/**
 * Init a vnode.
 */
void fs_vnode_init(vnode_t * vnode, ino_t vn_num, struct fs_superblock * sb,
                   const vnode_ops_t * const vnops);

/**
 * Returns the refcount of a vnode.
 */
int vrefcnt(struct vnode * vnode);

void vrefset(vnode_t * vnode, int refcnt);

/**
 * Increment the vn_refcount field of a vnode.
 * Each vnode maintains a reference count of how many parts of the system are
 * using the vnode. This allows the system to detect when a vnode is no longer
 * being used and can be safely recycled or freed.
 *
 * Any code in the system which will maintain a reference to a vnode
 * (after a function call) should call vref().
 * @return 0 if referenced; -ENOLINK if ref failed.
 */
int vref(vnode_t * vnode);

/** @addtogroup vput, vrele, vunref
 * Decrement the refcount for a vnode.
 * @{
 */

/**
 * Decrement the vn_refcount field of a vnode.
 * The function takes an unlocked vnode and returns with the vnode
 * unlocked.
 */
void vrele(vnode_t * vnode);

/**
 * Decrement the vn_refcount field of a vnode.
 * The function should  be given a locked vnode as argument, the vnode
 * is unlocked after the function returned.
 */
void vput(vnode_t * vnode);

/**
 * Decrement the vn_refcount field of a vnode.
 * The function takes a locked vnode as argument, and returns with
 * the vnode locked.
 */
void vunref(vnode_t * vnode);

/**
 * @}
 */

/**
 * Cleanup some vnode data.
 * File system is responsible to call this function before deleting a vnode.
 * This handles following cleanup tasks:
 * - Release and write out buffers
 */
void fs_vnode_cleanup(vnode_t * vnode);

#endif /* FS_H */

/**
 * @}
 */
