/**
 *******************************************************************************
 * @file    fz_fs.c
 * @author  Olli Vanhoja
 * @brief   File System wrapper for FatFs.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#define KERNEL_INTERNAL 1
#include <autoconf.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/hash.h>
#include <kinit.h>
#include <kstring.h>
#include <fs/vfs_hash.h>
#include <kmalloc.h>
#include <proc.h>
#include "fatfs.h"

static int fatfs_mount(const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb);
static int fatfs_delete_vnode(vnode_t * vnode);
static int fatfs_lookup(vnode_t * dir, const char * name, size_t name_len,
                        vnode_t ** result);
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             long vn_hash, fs_superblock_t * sb);
static void get_mp_stat(vnode_t * vnode, struct stat * st);

static struct fs fatfs_fs = {
    .fsname = FATFS_FSNAME,
    .mount = fatfs_mount,
    .umount = NULL,
    .sbl_head = 0
};

/**
 * Array of all FAT mounts for faster access.
 */
struct fatfs_sb ** fatfs_sb_arr;

const vnode_ops_t fatfs_vnode_ops = {
    .lookup = fatfs_lookup,
};

GENERATE_INSERT_SB(fatfs_sb, fatfs_fs)

int fatfs_init(void) __attribute__((constructor));
int fatfs_init(void)
{
    SUBSYS_DEP(proc_init);
    SUBSYS_INIT("fatfs");

    fatfs_sb_arr = kcalloc(configFATFS_MAX_MOUNTS, sizeof(struct fatfs_sb *));
    if (!fatfs_sb_arr)
        return -ENOMEM;

    fs_register(&fatfs_fs);

    return 0;
}

/**
 * Comparer for vfs_hash.
 */
static int fatfs_vncmp(struct vnode * vp, void * arg)
{
    const struct fatfs_inode * const in = get_inode_of_vnode(vp);
    const char * fpath = (char *)arg;

    return strcmp(in->in_fpath, fpath);
}

/**
 * Mount a new fatfs.
 * @param mode      mount flags.
 * @param param     contains optional mount parameters.
 * @param parm_len  length of param string.
 * @param[out] sb   Returns the superblock of the new mount.
 * @return error code, -errno.
 */
static int fatfs_mount(const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb)
{
    static dev_t fatfs_vdev_minor = 0;
    struct fatfs_sb * fatfs_sb = NULL;
    fs_superblock_t * sbp;
    vnode_t * vndev;
    int err, retval = 0;

    /* Get device vnode */
    err = lookup_vnode(&vndev, curproc->croot, source, 0);
    if (err) {
        retval = err;
        goto fail;
    }

    /* Allocate superblock */
    fatfs_sb = kcalloc(1, sizeof(struct fatfs_sb));
    if (!fatfs_sb) {
        retval = -ENOMEM;
        goto fail;
    }

    /* Init super block */
    sbp = &(fatfs_sb->sbn.sbl_sb);
    sbp->fs = &fatfs_fs;
    sbp->vdev_id = DEV_MMTODEV(FATFS_VDEV_MAJOR_ID, fatfs_vdev_minor++);
    sbp->mode_flags = mode;
    sbp->sb_dev = vndev;
    sbp->sb_hashseed = sbp->vdev_id;
    /* Function pointers to superblock methods */
    sbp->get_vnode = NULL; /* Not implemented for FAT. */
    sbp->delete_vnode = fatfs_delete_vnode;

    fatfs_sb->sbn.next = NULL; /* SB list */

    /* Mount */
    char pdrv = (char)DEV_MINOR(sbp->vdev_id);
    err = f_mount(fatfs_sb->ff_fs, &pdrv, 1);
    switch (err) {
    case FR_INVALID_DRIVE:
        retval = -ENXIO;
        goto fail;
    case FR_DISK_ERR:
        retval = -EIO;
        goto fail;
    case FR_NOT_READY:
        retval = -EBUSY;
        goto fail;
    case FR_NO_FILESYSTEM:
        retval = -EINVAL;
        goto fail;
    default:
        if (err) {
            /* Unknown error */
            retval = -EIO;
            goto fail;
        }
    }

    fatfs_sb_arr[DEV_MINOR(sbp->vdev_id)] = fatfs_sb;
    /* Add this sb to the list of mounted file systems. */
    insert_superblock(fatfs_sb);

    goto out;
fail:
    kfree(fatfs_sb);
out:
    *sb = &(fatfs_sb->sbn.sbl_sb);
    return retval;
}

static int fatfs_delete_vnode(vnode_t * vnode)
{
    /* TODO */
    return -ENOTSUP;
}

/**
 * Lookup for a vnode (file/dir) in FatFs.
 * First lookup form vfs_hash and if not found then read it from ff which will
 * probably read it via devfs interface. After the vnode has been created it
 * will be added to the vfs hashmap. In ff terminology all files and directories
 * that are in hashmap are also open on a file/dir handle, thus we'll have to
 * make sure we don't have too many vnodes in cache that have no references, to
 * avoid hitting any ff hard limits.
 */
static int fatfs_lookup(vnode_t * dir, const char * name, size_t name_len,
                        vnode_t ** result)
{
    struct fatfs_inode * indir = get_inode_of_vnode(dir);
    struct fatfs_sb * sb = get_rfsb_of_sb(dir->sb);
    char * in_fpath;
    size_t in_fpath_size;
    long vn_hash;
    struct vnode * vn = NULL;
    /* Following variables are used only if not cached. */
    struct fatfs_inode * in = NULL;
    ino_t num;
    mode_t vn_mode;
    /* --- */
    int err, retval = 0;

    /* Format full path */
    in_fpath_size = name_len + strlenn(indir->in_fpath, NAME_MAX + 1) + 6;
    in_fpath = kmalloc(in_fpath_size);
    if (!in_fpath)
        return -ENOMEM;
    ksprintf(in_fpath, in_fpath_size, "%u:/%s/%s",
            DEV_MINOR(indir->in_vnode.sb->vdev_id), indir->in_fpath, name);

    vn_hash = hash32_str(in_fpath, 0);

    /* Lookup from vfs_hash */
    err = vfs_hash_get(
            dir->sb,        /* FS superblock */
            vn_hash,        /* Hash */
            &vn,            /* Retval */
            fatfs_vncmp,    /* Comparer */
            in_fpath        /* Compared fpath */
          );
    if (err) {
        retval = -EIO;
        goto fail;
    }
    if (vn) {
        /* Found it */
        kfree(in_fpath);
        vref(vn);
        *result = vn;

        goto out;
    }

    /*
     * Create a inode and fetch data from the device.
     */

    in = kcalloc(1, sizeof(struct fatfs_inode));
    if (!in) {
        retval = -ENOMEM;
        goto fail;
    }
    in->in_fpath = in_fpath;
    vn = &in->in_vnode;

    /* Try open */
    vn_mode = S_IFDIR;
    err = f_opendir(&in->dp, in->in_fpath);
    if (err == FR_NO_PATH) {
       /* Probably not a dir, try to open as a file. */
        vn_mode = S_IFREG;
        err = f_open(&in->fp, in->in_fpath, FA_OPEN_EXISTING);
    }
    switch (err) {
    case FR_NO_PATH:
        retval = -ENOENT;
        goto fail;
    case FR_DISK_ERR:
    case FR_INT_ERR:
    case FR_INVALID_OBJECT:
        retval = -EIO;
        goto fail;
    case FR_NOT_READY:
        retval = -EBUSY;
        goto fail;
    case FR_INVALID_NAME:
    case FR_INVALID_DRIVE:
        retval = -EINVAL;
        goto fail;
    case FR_NOT_ENABLED:
    case FR_NO_FILESYSTEM:
        retval = -ENODEV;
        goto fail;
    case FR_TIMEOUT:
        retval = -EWOULDBLOCK;
        goto fail;
    case FR_NOT_ENOUGH_CORE:
        retval = -ENOMEM;
        goto fail;
    case FR_TOO_MANY_OPEN_FILES:
        retval = -ENFILE;
        goto fail;
    default:
        if (err) {
            /* Unknown error */
            retval = -EIO;
            goto fail;
        }
    }

    num = sb->ff_ino++;
    init_fatfs_vnode(vn, num, vn_mode, vn_hash, &(sb->sbn.sbl_sb));

    /* Insert to cache */
    vnode_t * xvp; /* Ex vp ? */
    vfs_hash_insert(vn, vn_hash, &xvp, fatfs_vncmp, in_fpath);
    if (err) {
        retval = -ENOMEM;
        goto fail;
    }
    if (xvp) {
        char msgbuf[80];

        /* TODO No idea what to do now */
        ksprintf(msgbuf, sizeof(msgbuf),
                "fatfs_lookup(): Found it during insert: \"%s\"\n",
                in_fpath);
        KERROR(KERROR_WARN, msgbuf);
    }

    *result = &in->in_vnode;
    vref(*result);
    goto out;
fail:
    kfree(in_fpath);
    kfree(in);
out:
    return retval;
}

ssize_t fatfs_write(file_t * file, const void * buf, size_t count)
{
    return -ENOTSUP;
}

ssize_t fatfs_read(file_t * file, void * buf, size_t count)
{
    return -ENOTSUP;
}

int fatfs_create(vnode_t * dir, const char * name, size_t name_len, mode_t mode,
                 vnode_t ** result)
{
    return -ENOTSUP;
}

int fatfs_mknod(vnode_t * dir, const char * name, size_t name_len, int mode,
                void * specinfo, vnode_t ** result)
{
    return -ENOTSUP;
}

int fatfs_link(vnode_t * dir, vnode_t * vnode, const char * name,
               size_t name_len)
{
    return -ENOTSUP;
}

int fatfs_unlink(vnode_t * dir, const char * name, size_t name_len)
{
    return -ENOTSUP;
}

int fatfs_mkdir(vnode_t * dir,  const char * name, size_t name_len, mode_t mode)
{
    return -ENOTSUP;
}

int fatfs_rmdir(vnode_t * dir,  const char * name, size_t name_len)
{
    return -ENOTSUP;
}

int fatfs_readdir(vnode_t * dir, struct dirent * d, off_t * off)
{
    struct fatfs_inode * in = get_inode_of_vnode(dir);
    FILINFO fno;
    char * fname;
    size_t fname_maxsize;
    int err;

    if (!S_ISDIR(dir->vn_mode))
        return -ENOTDIR;

#if configFATFS_USE_LFN
    fname_maxsize = _MAX_LFN + 1;
    fno.lfsize = fname_maxsize;
    fno.lfname = kmalloc(fname_maxsize);
#else
    fname_maxsize = 13;
#endif

    err = f_readdir(&in->dp, &fno);
    switch (err) {
    case FR_DISK_ERR:
    case FR_INT_ERR:
    case  FR_INVALID_OBJECT:
        return -EIO;
    case FR_NOT_READY:
        return -EBUSY;
    case FR_TIMEOUT:
        return -EWOULDBLOCK;
    case FR_NOT_ENOUGH_CORE:
        return -ENOMEM;
    default:
        if (err) {
            /* Unknown error */
            return -EIO;
        }
    }

    d->d_ino = 0; /* TODO */
#if configFATFS_USE_LFN
    fname = *fno.lfname ? fno.lfname : fno.fname;
#else
    fname = fno.fname;
#endif
    strlcpy(d->d_name, fname, fname_maxsize);

    return 0;
}

int fatfs_stat(vnode_t * vnode, struct stat * buf)
{
    struct fatfs_inode * in = get_inode_of_vnode(vnode);
    FILINFO fno;
    struct stat mp_stat;
    int err;

    get_mp_stat(vnode, &mp_stat);

    err = f_stat(in->in_fpath, &fno);
    switch (err) {
    case FR_DISK_ERR:
    case FR_INT_ERR:
        return -EIO;
    case FR_NOT_READY:
        return -EBUSY;
    case FR_NO_FILE:
    case FR_NO_PATH:
        return -ENOENT;
    case FR_INVALID_NAME:
    case  FR_INVALID_DRIVE:
        return -EINVAL;
    case FR_NOT_ENABLED:
    case FR_NO_FILESYSTEM:
        return -ENODEV;
    case FR_TIMEOUT:
        return -EWOULDBLOCK;
    case FR_NOT_ENOUGH_CORE:
        return -ENOMEM;
    default:
        if (err) {
            /* Unknown error */
            return -EIO;
        }
    }

    buf->st_dev = vnode->sb->vdev_id;
    buf->st_ino = vnode->vn_num;
    buf->st_mode = vnode->vn_mode;
    buf->st_nlink = 1; /* Always one link on FAT. */
    buf->st_uid = mp_stat.st_uid;
    buf->st_gid = mp_stat.st_gid;
    buf->st_size = fno.fsize;
    /* TODO Times */
#if 0
    buf->st_atime;
    buf->st_mtime;
    buf->st_ctime;
    buf->st_birthtime;
#endif
    buf->st_flags = fno.fattrib & (AM_RDO | AM_HID | AM_SYS | AM_ARC);
    buf->st_blksize = 512;
    buf->st_blocks = fno.fsize / 512 + 1; /* Best guess. */

    return 0;
}

int fatfs_chmod(vnode_t * vnode, mode_t mode)
{
    /* TODO */
    return -ENOTSUP;
}

int fatfs_chown(vnode_t * vnode, uid_t owner, gid_t group)
{
    return -ENOTSUP;
}

/**
 * Initialize fatfs vnode data.
 * @param vnode is the target vnode to be initialized.
 */
static void init_fatfs_vnode(vnode_t * vnode, ino_t inum, mode_t mode,
                             long vn_hash, fs_superblock_t * sb)
{
    struct stat stat;

    fs_vnode_init(vnode, inum, sb, &fatfs_vnode_ops);

    vnode->vn_hash = vn_hash;
    /* TODO Correct modes */
    if (mode & S_IFDIR)
        mode |= S_IRWXU | S_IXGRP | S_IXOTH;
    vnode->vn_mode = mode | S_IRGRP | S_IROTH;

    fatfs_stat(vnode, &stat);
    vnode->vn_len = stat.st_size;
}

/**
 * Get mountpoint stat.
 */
static void get_mp_stat(vnode_t * vnode, struct stat * st)
{
    vnode->sb->mountpoint->vnode_ops->stat(vnode->sb->mountpoint, st);
}
