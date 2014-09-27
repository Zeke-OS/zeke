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
#include <sys/hash.h>
#include <kinit.h>
#include <kstring.h>
#include <fs/vfs_hash.h>
#include <kmalloc.h>
#include <proc.h>
#include "fatfs.h"

static int fatfs_mount(const char * source, uint32_t mode,
        const char * parm, int parm_len, struct fs_superblock ** sb);
static int fatfs_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                           vnode_t ** vnode);
static int fatfs_delete_vnode(vnode_t * vnode);
static int fatfs_lookup(vnode_t * dir, const char * name, size_t name_len,
                        vnode_t ** result);


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
    sbp->get_vnode = fatfs_get_vnode;
    sbp->delete_vnode = fatfs_delete_vnode;

    fatfs_sb->sbn.next = NULL; /* SB list */

    /* Mount */
    char pdrv = (char)DEV_MINOR(sbp->vdev_id);
    err = f_mount(fatfs_sb->ff_fs, &pdrv, 1);
    if (err == FR_INVALID_DRIVE)
        retval = -ENXIO;
    else if (err == FR_DISK_ERR)
        retval = -EIO;
    else if (err == FR_NOT_READY)
        retval = -EBUSY;
    else if (err == FR_NO_FILESYSTEM)
        retval = -EINVAL;
    else if (err) /* Unknown error */
        retval = -EIO;
    if (err)
        goto fail;

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

static int fatfs_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                           vnode_t ** vnode)
{
    /* TODO */
}

static int fatfs_delete_vnode(vnode_t * vnode)
{
    /* TODO */
}

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

    /* Create a inode and fetch data from the device. */

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
    if (err == FR_NO_PATH)
        retval = -ENOENT;
    else if (err ==  FR_DISK_ERR)
        retval = -EIO;
    else if (err == FR_INT_ERR)
        retval = -ENOTRECOVERABLE;
    else if (err == FR_NOT_READY)
        retval = -EBUSY;
    else if (err == FR_INVALID_NAME)
        retval = -EINVAL;
    else if (err == FR_INVALID_OBJECT)
        retval = -ENOTRECOVERABLE;
    else if (err == FR_INVALID_DRIVE)
        retval = -EINVAL;
    else if (err == FR_NOT_ENABLED || err == FR_NO_FILESYSTEM)
        retval = -ENODEV;
    else if (err == FR_TIMEOUT)
        retval = -EWOULDBLOCK;
    else if (err == FR_NOT_ENOUGH_CORE)
        retval = -ENOMEM;
    else if (err == FR_TOO_MANY_OPEN_FILES)
        retval = -ENFILE;
    else if (err) /* Unknown error */
        retval = -EIO;
    if (err)
        goto fail;

    num = sb->ff_ino++;
    fs_vnode_init(vn, num, &(sb->sbn.sbl_sb), &fatfs_vnode_ops);
    vn->vn_len = 0;
    vn->vn_hash = vn_hash;
    /* TODO Correct modes */
    if (vn_mode & S_IFDIR)
        vn_mode |= S_IRWXU | S_IXGRP | S_IXOTH;
    vn->vn_mode = vn_mode | S_IRGRP | S_IROTH;
    /* TODO Times */

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
