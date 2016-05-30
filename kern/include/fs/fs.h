/**
 *******************************************************************************
 * @file    fs.h
 * @author  Olli Vanhoja
 * @brief   Virtual file system headers.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <dirent.h>
#include <limits.h>
#include <machine/atomic.h>
#include <mount.h>
#include <stdint.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/tree.h>
#include <sys/types.h>
#include <klocks.h>
#include <kobj.h>
#include <uio.h>

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
#define FS_TFLAGS_ALLOF(act_flags, exp_flags) \
    ((act_flags & exp_flags) == exp_flags)

/* End of macros **************************************************************/

struct cred;
struct proc_info;

/*
 * Types for buffer pointer storage object in vnode.
 */
SPLAY_HEAD(bufhd_splay, buf);
struct bufhd {
    struct bufhd_splay sroot;
};

typedef struct vnode {
    ino_t vn_num;               /*!< vnode number. */
    unsigned vn_hash;           /*!< Hash for using vfs hashing. */
    atomic_t vn_refcount;

    /**
     * Pointer to the next vnode in mounted file system.
     * If no fs is mounted on this vnode then this is a self pointing pointer.
     */
    struct vnode * vn_next_mountpoint;
    /**
     * Pointer to the previous mountpoint vnode.
     */
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

#define VN_LOCK_TYPE    MTX_TYPE_TICKET
#define VN_LOCK_OPT     MTX_OPT_SLEEP
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
    int oflags;         /*!< File status flags. */
    vnode_t * vnode;
    void * stream;      /*!< Pointer to a special file stream data or info. */
    struct kobj f_obj;
} file_t;

/**
 * Open file descriptors.
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

/*
 * Macros for fs giant locks.
 */
#define FS_GIANT_TYPE    MTX_TYPE_TICKET
#define FS_GIANT_OPT     0
#define FS_GIANT_INIT(_x_) mtx_init(_x_, FS_GIANT_TYPE, FS_GIANT_OPT)

/**
 * File system.
 */
typedef struct fs {
    char fsname[8];
    mtx_t fs_giant;

    /**
     * Mount a new super block of this fs type.
     * @param source    is a pointer to the source path/URI if applicable to
     *                  the file system.
     * @param mode      contains MNT_ mount flags defined in mount.h.
     * @param parm      is a pointer to file system specific parameters, parm
     *                  doesn't need to be C string.
     * @param parm_len  is the size of parm in bytes.
     * @param[out] sb   is a pointer to a pointer where a pointer to the
     *                  resulting super block will be returned.
     */
    int (*mount)(const char * source, uint32_t mode,
                 const char * parm, int parm_len, struct fs_superblock ** sb);

    /**
     * List of all mounts of this fs type.
     */
    SLIST_HEAD(sb_list, fs_superblock) sblist_head;
    SLIST_ENTRY(fs) _fs_list;
} fs_t;

/**
 * File system superblock.
 */
struct fs_superblock {
    fs_t * fs;              /*!< A pointer to the file system implementation. */
    dev_t vdev_id;          /*!< Virtual dev_id. */
#ifdef configVFS_HASH
    unsigned sb_hashseed;   /*!< Seed for using vfs hashing. */
#endif
    uint32_t mode_flags;    /*!< Mount mode flags. */
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
    int (*get_vnode)(struct fs_superblock * sb, ino_t * vnode_num,
                     vnode_t ** vnode);

    /**
     * Delete a vnode reference.
     * Deletes a reference to a vnode and destroys the inode corresponding to
     * the inode data in memory if there is no more links and references to it.
     * @param[in] vnode is the vnode.
     * @return Returns 0 if no error; Otherwise value other than zero.
     */
    int (*delete_vnode)(vnode_t * vnode);

    /**
     * Unmount the file system.
     * @param this_sb is a pointer to the calling superblock.
     * @return  Returns 0 if the file system superblock was unmounted;
     *          Otherwise a negative errno codes is returned.
     *
     */
    int (*umount)(struct fs_superblock * this_sb);

    SLIST_ENTRY(fs_superblock) _sblist;
};

/**
 * vnode operations struct.
 * These are usually defined per file system type but some operations might
 * be inherited from other file sytems and ultimately if no new implementation
 * is provided the function shall be inherited from nofs. Inherit should be
 * done by calling fs_inherit_vnops() declared in fs_util.h.
 */
typedef struct vnode_ops {
    /* Operation for open files
     * ------------------------ */
    int (*lock)(file_t * file);
    int (*release)(file_t * file);
    /**
     * Read transfers bytes from file into buf.
     * @param file      is a file stored in the file system.
     * @param count     is the number of bytes to be read.
     * @return  Returns the number of bytes read;
     *          Otherwise a negative errno code is returned.
     */
    ssize_t (*read)(file_t * file, struct uio * uio, size_t count);
    /**
     * Write transfers bytes from buf into file.
     * Writing is begin from offset and ended at offset + count. buf must
     * therefore contain at least count bytes. If offset is past end of the
     * current file the file will be extended; If offset is smaller than file
     * length, the existing data will be overwriten.
     * @param file      is a file stored in the file system.
     * @param count     is the number of bytes buf contains.
     * @return  Returns the number of bytes written;
     *          Otherwise a negative errno code is returned.
     */
    ssize_t (*write)(file_t * file, struct uio * uio, size_t count);
    /**
     * Seek a file.
     * @param file      is a file stored in the file system.
     * @param offset    is the seek offset.
     * @param whence    is the location to start seeking from either
     *                  SEEK_SET, SEEK_CUR, or SEEK_END.
     */
    off_t (*lseek)(file_t * file, off_t offset, int whence);
    /**
     * IO Control.
     * Only defined for devices and shall point to fs_enotsup_ioctl() if not
     * supported.
     * @param file      is the open file accessed.
     * @param request   is the request number.
     * @param arg       is a pointer to the argument (struct).
     * @param ar_len    is the length of arg.
     * @return          0 if succeed;
     *                  Otherwise a negative errno code is returned.
     */
    int (*ioctl)(file_t * file, unsigned request, void * arg, size_t arg_len);
    /* Event handlers
     * -------------- */
    /**
     * Vnode opened callback.
     * Vnode was opened in a syscall by a process to create a file descriptor
     * for it.
     * @param p     is the process that called fs_fildes_create_curproc() for
     *              file.
     * @param vnode is the vnode that will be opened and referenced by
     *              a new file descriptor.
     * @return      Default action is to return 0 but negative errno can be
     *              returned to indicate an error and thus cancel the file
     *              opening procedure.
     */
    int (*event_vnode_opened)(struct proc_info * p, vnode_t * vnode);
    /**
     * File descriptor created for the previously opened file.
     * @note        Opening the file nor file descriptor creation can't be
     *              cancelled anymore at this point, if a file system
     *              needs to be able to cancel the opening procedure that
     *              must be done by registering a event_vnode_opened() function.
     * @param p     is the process that owns the new file descriptor.
     * @param file  is the new file descriptor.
     */
    void (*event_fd_created)(struct proc_info * p, file_t * file);
    /**
     * File closed callback.
     * This function is called whenever a process closes a file. This function
     * is called before actual file descriptor close operation is commited,
     * allowing fs driver to access fd for the last time.
     *
     * We don't like file opening and closing because we believe a file system
     * is a database like thing not a damn tape drive. Reason to have these fd
     * event callback functions is mainly to handle TTY connections nicely.
     * @note Process closing a file might not be the only process helding
     *       a reference to the file.
     * @param p     is the process that called fs_fildes_close_curproc() for
     *              file.
     * @param file  is the file that was closed by p.
     */
    void (*event_fd_closed)(struct proc_info * p, file_t * file);
    /* Directory file operations
     * ------------------------- */
    /**
     * Create a new vnode with S_IFREG and a hard link with specified name for
     * it created in dir.
     * @param dir       is the directory vnode which is used to store
     *                  the hard link created.
     * @param name      is the name of the hard link.
     * @param[out] result is a pointer to the resulting vnode.
     * @return  Zero in case of operation succeed;
     *          Otherwise a negative errno code is returned.
     */
    int (*create)(vnode_t * dir, const char * name, mode_t mode,
                  vnode_t ** result);
    /**
     * Create a special vnode.
     * @note vn_ops must be set by the caller manually after creation of
     *       the vnode.
     * @param specinfo  is a pointer to the special info struct.
     * @param mode      is the mode of the new file.
     * @param specinfo  is a pointer to a optional data that can be access via
     *                  the vnode, can be set to NULL.
     * @param[out] result will return the resulting vnode.
     * @return  Zero in case of operation succeed;
     *          Otherwise a negative errno code is returned.
     */
    int (*mknod)(vnode_t * dir, const char * name, int mode, void * specinfo,
                 vnode_t ** result);
    /**
     * Lookup for a hard linked vnode in a directory vnode.
     * The ref count of result should be incremented by the underlying fs
     * implementation.
     * @param dir       is a directory in the file system.
     * @param name      is the filename to be searched for.
     * @param[out] result will be set to the result of the lookup.
     * @return  Returns 0 if a vnode was found;
     *          If vnode is same as dir (root dir) -EDOM shall be returned;
     *          Otherwise a negative errno code is returned.
     */
    int (*lookup)(vnode_t * dir, const char * name, vnode_t ** result);
    /**
     * Reverse lookup for a hard link name by its inode number.
     * @param dir       is a directory in the file system.
     * @param ino       is the inode number.
     * @param[out] name is a buffer for the file name.
     * @param name_len  is the size of the name buffer in bytes.
     * @return Returns 0 if a vnode was found.
     */
    int (*revlookup)(vnode_t * dir, ino_t * ino, char * name, size_t name_len);
    /**
     * Create a hard link.
     * Link vnode into dir with the specified name.
     * @param dir       is the directory where entry will be created.
     * @param vnode     is a vnode where the link will point.
     * @param name      is the name of the hard link.
     * @return  Returns 0 if creating a link succeeded;
     *          Otherwise a negative errno code is returned.
     */
    int (*link)(vnode_t * dir, vnode_t * vnode, const char * name);
    /**
     * Unlink a hard link.
     * Unlink a hard link in the directory specified.
     * @param dir       is the directory where the vnode is linked to.
     * @param name      is the name of the link in the directory.
     * @return  Returns 0 if the link was removed;
     *          Otherwise a negative errno code is returned.
     */
    int (*unlink)(vnode_t * dir, const char * name);
    /**
     * Create a directory called name in dir.
     * Implementation of mkdir() shall also set uid and gid of the new directory
     * if the underlying filesystem supports that feature.
     * @param dir       is a directory in the file system.
     * @param name      is the name of the new directory.
     * @param mode      is the file mode of the new directory.
     * @return  Returns zero if a new directory wit the specified name was
     *          created; Otherwise a negative errno code is returned.
     */
    int (*mkdir)(vnode_t * dir,  const char * name, mode_t mode);
    /**
     * Remove a directory.
     * Shall fail if named directory is a mountpoint to another file system.
     * @param dir       is a directory in the file system.
     * @param name      is the name of the directory to be removed.
     * @return  Returns 0 if the directory was removed;
     *          Otherwise a negative errno code is returned.
     */
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
    /* Operations specified for any file type
     * -------------------------------------- */
    /**
     * Get file status.
     * @param vnode     is a pointer to a vnode existing in the file system.
     * @param[out] buf  is a pointer to a buffer that can hold struct stat for
     *                  the vnode.
     * @return  Returns 0 if stat was written to buf;
     *          Otherwise a negative errno code is returned.
     */
    int (*stat)(vnode_t * vnode, struct stat * buf);
    /**
     * Set file access and modification times.
     * @param vnode     is a pointer to a vnode existing in the file system.
     * @return  Returns 0 if utimes were changed succefully;
     *          Otherwise a negative errno code is returned.
     */
    int (*utimes)(vnode_t * vnode, const struct timespec times[2]);
    /**
     * Change file mode.
     * @param vnode     is a pointer to a vnode existing in the file system.
     */
    int (*chmod)(vnode_t * vnode, mode_t mode);
    /**
     * Change file flags.
     * @param vnode     is a pointer to a vnode existing in the file system.
     */
    int (*chflags)(vnode_t * vnode, fflags_t flags);
    /**
     * Change file owner and group.
     * @param vnode     is a pointer to a vnode existing in the file system.
     */
    int (*chown)(vnode_t * vnode, uid_t owner, gid_t group);
} vnode_ops_t;

/** vnops for not supported operations */
extern const vnode_ops_t nofs_vnode_ops;

/**
 * VFS error logging.
 * @{
 */

#define FS_KERROR_FS_FMT(_STR_) "%pF::%s: " _STR_
#define FS_KERROR_VNODE_FMT(_STR_) "%pV::%s: " _STR_

/**
 * KERROR for fs.
 */
#define FS_KERROR_FS(_LVL_, _fs_, _FMT_, ...)   \
    KERROR(_LVL_, FS_KERROR_FS_FMT(_FMT_),      \
           (_fs_), __func__, ##__VA_ARGS__)

/**
 * KERROR for vnode.
 */
#define FS_KERROR_VNODE(_LVL_, _vn_, _FMT_, ...)        \
    KERROR(_LVL_, FS_KERROR_VNODE_FMT(_FMT_),           \
           (_vn_), __func__, ##__VA_ARGS__)

/**
 * @}
 */

/* VFS Function Prototypes */

/**
 * Register a new file system driver.
 * @param fs file system control struct.
 */
int fs_register(fs_t * fs);

/**
 * Find registered file system by name.
 * @param fsname is the name of the file system.
 * @return The file system structure.
 */
fs_t * fs_by_name(const char * fsname);

/**
 * Iterate over all file system drivers.
 * @param fsp shall be null on start and shall be given the value that was
 *            returned on last call of this function.
 */
fs_t * fs_iterate(fs_t * fsp);

/**
 * Mount file system.
 * @param target    is the target mount point directory.
 * @param source    is a pointer to the source path/URI if applicable to
 *                  the file system.
 * @param fsname    is the name of the file system type to mount. This
 *                  argument is optional and if left out fs_mount() tries
 *                  to determine the file system type from existing information.
 * @param flags     contains MNT_ mount flags defined in mount.h.
 * @param parm      is a pointer to file system specific parameters, parm
 *                  doesn't need to be C string.
 * @param parm_len  is the size of parm in bytes.
 */
int fs_mount(vnode_t * target, const char * source, const char * fsname,
             uint32_t flags, const char * parm, int parm_len);

/**
 * Unmount a file system superblock.
 * @param sb is a pointer to a superblock of a file system.
 */
int fs_umount(struct fs_superblock * sb);

/**
 * Lookup for a vnode by path.
 * @param[out]  result  is the target where vnode struct is stored.
 * @param[in]   root    is the vnode where search is started from.
 * @param       str     is the path.
 * @param       oflags  Tested flags are: O_DIRECTORY, O_NOFOLLOW
 * @return  Returns zero if vnode was found;
 *          error code -errno in case of an error.
 */
int lookup_vnode(vnode_t ** result, vnode_t * root, const char * str,
                 int oflags);

/**
 * Walks the file system for a process and tries to locate vnode corresponding
 * to a given path.
 * @param fd        is an optional starting point for relative search.
 * @param path      is a pointer to the path C string.
 * @param atflags   if this is set to AT_FDARG then fd is used;
 *                  AT_FDCWD is implicit rule for this function.
 *                  AT_SYMLINK_NOFOLLOW is optional and AT_SYMLINK_FOLLOW is
 *                  implicit.
 */
int fs_namei_proc(vnode_t ** result, int fd, const char * path, int atflags);

int chkperm(struct stat * stat, const struct cred * cred, int oflags);

/**
 * Check file permissions against oflag(s).
 * This function accepts both oflags and amodes, except F_OK.
 * @param stat      is a pointer to stat struct of the file.
 * @param oflags    specifies the verified oflags or amodes.
 * @return  Returns negative errno in case of error or improper permissions;
 *          Otherwise zero.
 */
int chkperm_curproc(struct stat * stat, int oflags);

/**
 * Check permissions to a vnode by curproc.
 * @param vnode is the vnode to be checked.
 * @param oflags specfies operation(s).
 */
int chkperm_vnode_curproc(vnode_t * vnode, int oflags);

/**
 * Check permissions to a vnode by given euid and egid.
 */
int chkperm_vnode(vnode_t * vnode, struct cred * cred, int oflags);

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
int fs_fildes_create_curproc(vnode_t * vnode, int oflags);

/**
 * Get next free file descriptor for the current process.
 * @param new_file  is the file to be stored in the next free position from
 *                  start.
 * @param start     is the start offset.
 */
int fs_fildes_curproc_next(file_t * new_file, int start);

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
int fs_fildes_close(struct proc_info * p, int fildes);

/**
 * Close all file descriptors above fildes_begin.
 */
void fs_fildes_close_all(struct proc_info * p, int fildes_begin);

/**
 * Close files marked with FD_CLOEXEC and O_CLOEXEC.
 * @param p is a pointer to the process.
 */
void fs_fildes_close_exec(struct proc_info * p);

/**
 * Test if a file is a tty.
 * @return  0 if the file is a TTY;
 *          Otherwise -ENOTTY is returned.
 */
int fs_fildes_isatty(int fd);

/**
 * Allocate a files_t struct for storing file descriptors.
 * @param nrfiles is the maximum number of files open.
 * @param umask is the default umask.
 */
files_t * fs_alloc_files(size_t nr_files, mode_t umask);

/**
 * Create a new file by using fs specific create() function.
 * File will be created relative to the attributes of a current process.
 * @param[in]   path    is a path to the new file.
 * @param[in]   mode    selects the file mode.
 * @param[out]  result  is the new vnode created.
 * @return      0 if succeed; Negative error code representing errno in case of
 *              failure.
 */
int fs_creat_curproc(const char * path, mode_t mode, vnode_t ** result);

/**
 * Create a new link to existing vnode.
 * @param fd1           fildes or AT_FDCWD
 * @param fd            fildes or AT_FDCWD
 * @param atflags       at flags.
 */
int fs_link_curproc(int fd1, const char * path1,
                    int fd2, const char * path2,
                    int atflags);

/**
 * Remove link to a file.
 * @param fd            is a file descriptor number or AT_FDCWD.
 * @param path          is a path to the file to be unlinked.
 * @param atflags       at flags.
 */
int fs_unlink_curproc(int fd, const char * path, int atflags);

/**
 * Remove link to a file relative to fd.
 */
int fs_unlinkat_curproc(int fd, const char * path, int flag);

/**
 * Create a new directory relative to the current process.
 */
int fs_mkdir_curproc(const char * pathname, mode_t mode);

/**
 * Remove a directory.
 */
int fs_rmdir_curproc(const char * pathname);

/**
 * Set file access and modification times.
 */
int fs_utimes_curproc(int fildes, const struct timespec times[2]);

/**
 * Change mode of a file.
 */
int fs_chmod_curproc(int fildes, mode_t mode);

/**
 * Change file flags.
 */
int fs_chflags_curproc(int fildes, fflags_t flags);

/**
 * Change owener and group of a file.
 */
int fs_chown_curproc(int fildes, uid_t owner, gid_t group);

/**
 * Returns the refcount of a vnode.
 */
int vrefcnt(struct vnode * vnode);

/**
 * Set refcount of a vnode.
 */
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

/**
 * @addtogroup vput, vrele, vunref
 * Decrement the refcount for a vnode.
 * @{
 */

/**
 * Decrement the vn_refcount field of a vnode.
 * The function takes an unlocked vnode and returns with the vnode
 * unlocked.
 */
void vrele(vnode_t * vnode);

static inline void _vrele_auto(vnode_t ** vnode)
{
    vrele(*vnode);
}

/**
 * Attribute to enable automatic release of a vnode reference.
 */
#define vnode_autorele \
    __attribute__((cleanup(_vrele_auto)))

/**
 * Decrement the vn_refcount field of a vnode.
 * The function takes an unlocked vnode and returns with the vnode
 * unlocked.
 * The function doesn't call delete_vnode() if ref is zero or less.
 */
void vrele_nunlink(vnode_t * vnode);

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

/* Not sup vnops (in nofs.c) */
int fs_enotsup_lock(file_t * file);
int fs_enotsup_release(file_t * file);
ssize_t fs_enotsup_read(file_t * file, struct uio * uio, size_t count);
ssize_t fs_enotsup_write(file_t * file, struct uio * uio, size_t count);
off_t fs_enotsup_lseek(file_t * file, off_t offset, int whence);
int fs_enotsup_ioctl(file_t * file, unsigned request, void * arg,
                     size_t arg_len);
int fs_enotsup_event_vnode_opened(struct proc_info * p, vnode_t * vnode);
void fs_enotsup_event_fd_created(struct proc_info * p, file_t * file);
void fs_enotsup_event_fd_closed(struct proc_info * p, file_t * file);
int fs_enotsup_create(vnode_t * dir, const char * name, mode_t mode,
                      vnode_t ** result);
int fs_enotsup_mknod(vnode_t * dir, const char * name, int mode,
                     void * specinfo, vnode_t ** result);
int fs_enotsup_lookup(vnode_t * dir, const char * name, vnode_t ** result);
int nofs_revlookup(vnode_t * dir, ino_t * ino, char * name, size_t name_len);
int fs_enotsup_link(vnode_t * dir, vnode_t * vnode, const char * name);
int fs_enotsup_unlink(vnode_t * dir, const char * name);
int fs_enotsup_mkdir(vnode_t * dir,  const char * name, mode_t mode);
int fs_enotsup_rmdir(vnode_t * dir,  const char * name);
int fs_enotsup_readdir(vnode_t * dir, struct dirent * d, off_t * off);
int fs_enotsup_stat(vnode_t * vnode, struct stat * buf);
int fs_enotsup_utimes(vnode_t * vnode, const struct timespec times[2]);
int fs_enotsup_chmod(vnode_t * vnode, mode_t mode);
int fs_enotsup_chflags(vnode_t * vnode, fflags_t flags);
int fs_enotsup_chown(vnode_t * vnode, uid_t owner, gid_t group);

#endif /* FS_H */

/**
 * @}
 */
