/**
 *******************************************************************************
 * @file    devfs.c
 * @author  Olli Vanhoja
 * @brief   Device file system.
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

#include <errno.h>
#include <sys/ioctl.h>
#include <fs/dev_major.h>
#include <fs/devfs.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/ramfs.h>
#include <hal/core.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>

/**
 * Max tries in case of block read/write returns 0.
 */
#define RW_MAX_TRIES 3

static int devfs_mount(const char * source, uint32_t mode,
                       const char * parm, int parm_len,
                       struct fs_superblock ** sb);
static int devfs_umount(struct fs_superblock * fs_sb);
static void devfs_event_fd_created(struct proc_info * p, file_t * file);
static void devfs_event_fd_closed(struct proc_info * p, file_t * file);
static int dev_ioctl(file_t * file, unsigned request,
                     void * arg, size_t arg_len);

vnode_ops_t devfs_vnode_ops = {
    .read = dev_read,
    .write = dev_write,
    .lseek = dev_lseek,
    .ioctl = dev_ioctl,
    .event_fd_created = devfs_event_fd_created,
    .event_fd_closed = devfs_event_fd_closed,
};

static fs_t devfs_fs = {
    .fsname = DEVFS_FSNAME,
    .mount = devfs_mount,
    .sblist_head = SLIST_HEAD_INITIALIZER(),
};

/* There is only one devfs, but it can be mounted multiple times */
vnode_t * vn_devfs;

static int devfs_delete_vnode(vnode_t * vnode);

int __kinit__ devfs_init(void)
{
    struct fs_superblock * sb;

    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("devfs");

    FS_GIANT_INIT(&devfs_fs.fs_giant);

    /*
     * Inherit ramfs and override what's needed.
     */
    fs_inherit_vnops(&devfs_vnode_ops, &ramfs_vnode_ops);
    vn_devfs = fs_create_pseudofs_root(&devfs_fs, VDEV_MJNR_DEVFS);
    if (!vn_devfs)
        return -ENOMEM;

    /*
     * It's perfectly ok to set a new delete_vnode() function
     * as sb struct is always a fresh struct so altering it
     * doesn't really break functionality for anyone else than us.
     */
    sb = vn_devfs->sb;
    sb->delete_vnode = devfs_delete_vnode;
    sb->umount = devfs_umount;

    /*
     * Finally register our creation with fs subsystem.
     */
    fs_register(&devfs_fs);

    _devfs_create_specials();

    return 0;
}

static int devfs_mount(const char * source, uint32_t mode,
                       const char * parm, int parm_len,
                       struct fs_superblock ** sb)
{
    if (!(vn_devfs && vn_devfs->sb))
        return -ENODEV;
    *sb = vn_devfs->sb;
    return 0;
}

static int devfs_umount(struct fs_superblock * fs_sb)
{
    /* NOP, everything relevant is handled by the vfs. */

    return 0;
}

int make_dev(struct dev_info * devnfo, uid_t uid, gid_t gid, int perms,
             vnode_t ** result)
{
    vnode_t * vn;
    int err, mode = ((devnfo->block_size > 1) ? S_IFBLK : S_IFCHR) | perms;

    if (!vn_devfs->vnode_ops->lookup(vn_devfs, devnfo->dev_name, NULL))
        return -EEXIST;

    err = devfs_vnode_ops.mknod(vn_devfs, devnfo->dev_name, mode, devnfo, &vn);
    if (err)
        return err;

    /* Replace ops with our own */
    vn->vnode_ops = &devfs_vnode_ops;

    vn->vnode_ops->chown(vn, uid, gid);

    if (result)
        *result = vn;

    return 0;
}

void destroy_dev(vnode_t * vn)
{
    char name[NAME_MAX];

    vn_devfs->vnode_ops->revlookup(vn_devfs, &vn->vn_num, name, sizeof(name));
    vn_devfs->vnode_ops->unlink(vn_devfs, name);
}

static int devfs_delete_vnode(vnode_t * vnode)
{
    struct dev_info * devnfo = (struct dev_info *)vnode->vn_specinfo;
    int err;

    if (devnfo->delete_vnode_callback)
        devnfo->delete_vnode_callback(devnfo);
    err = ramfs_delete_vnode(vnode);

    return err;
}

static void devfs_event_fd_created(struct proc_info * p, file_t * file)
{
    struct dev_info * devnfo = (struct dev_info *)file->vnode->vn_specinfo;

    if (devnfo->open_callback)
        devnfo->open_callback(p, file, devnfo);
}

static void devfs_event_fd_closed(struct proc_info * p, file_t * file)
{
    struct vnode * vnode = file->vnode;
    struct dev_info * devnfo = (struct dev_info *)vnode->vn_specinfo;

    if (devnfo->close_callback)
        devnfo->close_callback(p, file, devnfo);
}

const char * devtoname(struct vnode * dev)
{
    struct dev_info * devnfo = (struct dev_info *)dev->vn_specinfo;

    if (!(dev->vn_mode & (S_IFBLK | S_IFCHR)))
        return NULL;

    return devnfo->dev_name;
}

ssize_t dev_read(file_t * file, struct uio * uio, size_t bcount)
{
    vnode_t * const vnode = file->vnode;
    const off_t offset = file->seek_pos;
    const int oflags = file->oflags;
    struct dev_info * devnfo = (struct dev_info *)vnode->vn_specinfo;
    uint8_t * buf;
    size_t buf_offset;
    off_t block_offset;
    ssize_t bytes_rd;
    int err;

    if (!devnfo->read)
        return -EOPNOTSUPP;

    err = uio_get_kaddr(uio, (void **)(&buf));
    if (err)
        return err;

    if ((devnfo->flags & DEV_FLAGS_MB_READ) &&
            ((bcount / devnfo->block_size) > 1)) {
        return devnfo->read(devnfo, offset, buf, bcount, oflags);
    }

    buf_offset = 0;
    block_offset = 0;
    do {
        int tries = RW_MAX_TRIES;
        size_t to_read = (bcount > devnfo->block_size) ?
            devnfo->block_size : bcount;
        ssize_t ret;

        while (1) {
            ret = devnfo->read(devnfo, offset + block_offset,
                               &buf[buf_offset], to_read, oflags);
            if (ret < 0 && --tries <= 0) {
                bytes_rd = (buf_offset > 0) ? buf_offset : ret;
                goto out;
            } else {
                break;
            }
        }

        buf_offset += ret;
        block_offset++;
        bcount -= ret;
    } while (bcount > 0);

    bytes_rd = buf_offset;
out:
    file->seek_pos += bytes_rd;
    return bytes_rd;
}

ssize_t dev_write(file_t * file, struct uio * uio, size_t bcount)
{
    vnode_t * const vnode = file->vnode;
    const off_t offset = file->seek_pos;
    const int oflags = file->oflags;
    struct dev_info * devnfo = (struct dev_info *)vnode->vn_specinfo;
    uint8_t * buf;
    size_t buf_offset;
    off_t block_offset;
    ssize_t bytes_wr;
    int err;

    if (!devnfo->write)
        return -EOPNOTSUPP;

    err = uio_get_kaddr(uio, (void **)(&buf));
    if (err)
        return err;

    if ((devnfo->flags & DEV_FLAGS_MB_WRITE) &&
            ((bcount / devnfo->block_size) > 1)) {
        return devnfo->write(devnfo, offset, buf, bcount, oflags);
    }

    buf_offset = 0;
    block_offset = 0;
    do {
        int tries = RW_MAX_TRIES;
        size_t to_write = (bcount > devnfo->block_size) ?
            devnfo->block_size : bcount;
        ssize_t ret;

        while (1) {
            ret = devnfo->write(devnfo, offset + block_offset,
                                &buf[buf_offset], to_write, oflags);
            if (ret < 0 && --tries <= 0) {
                bytes_wr = (buf_offset > 0) ? buf_offset : ret;
                goto out;
            } else {
                break;
            }
        }

        buf_offset += ret;
        block_offset++;
        bcount -= ret;
    } while (bcount > 0);

    bytes_wr = buf_offset;
out:
    file->seek_pos += bytes_wr;
    return bytes_wr;
}

off_t dev_lseek(file_t * file, off_t offset, int whence)
{
    struct dev_info * devnfo = (struct dev_info *)file->vnode->vn_specinfo;

    if (devnfo->lseek) {
        return devnfo->lseek(file, devnfo, offset, whence);
    } else {
        file->seek_pos = offset;
        return offset;
    }
}

static int dev_ioctl(file_t * file, unsigned request, void * arg, size_t arg_len)
{
    struct dev_info * devnfo = (struct dev_info *)file->vnode->vn_specinfo;

    if (!devnfo)
        return -ENOTTY;

    if (devnfo->ioctl) {
        int err;

        err = devnfo->ioctl(devnfo, request, arg, arg_len);
        if (err == 0 || err != -EINVAL)
            return err;
    }

    /*
     * Default handlers.
     */
    switch (request) {
    case IOCTL_GETBLKSIZE:
        if (!arg)
            return -EINVAL;

        sizetto(devnfo->block_size, arg, arg_len);

        return 0;
    case IOCTL_GETBLKCNT:
        if (!arg)
            return -EINVAL;

        sizetto(devnfo->num_blocks, arg, arg_len);

        return 0;
    case IOCTL_FLSBLKBUF: /* Don't care if the dev doesn't support sync. */
        return 0;
    default:
        return -EINVAL;
    }
}
