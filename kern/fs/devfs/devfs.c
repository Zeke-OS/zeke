/**
 *******************************************************************************
 * @file    block.c
 * @author  Olli Vanhoja
 * @brief
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

#include <hal/core.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <errno.h>
#include <kerror.h>
#include <fs/ramfs.h>
#include <fs/devfs.h>

/**
 * Max tries in case of block read/write returns 0.
 */
#define RW_MAX_TRIES 3

const vnode_ops_t devfs_vnode_ops = {
    .write = dev_write,
    .read = dev_read,
    .create = ramfs_create,
    .mknod = ramfs_mknod,
    .lookup = ramfs_lookup,
    .link = ramfs_link,
    .unlink = ramfs_unlink,
    .mkdir = ramfs_mkdir,
    .readdir = ramfs_readdir,
    .stat = ramfs_stat
};

static int devfs_mount(const char * source, uint32_t mode,
                       const char * parm, int parm_len,
                       struct fs_superblock ** sb);
static int devfs_umount(struct fs_superblock * fs_sb);

static fs_t devfs_fs = {
    .fsname = DEVFS_FSNAME,
    .mount = devfs_mount,
    .umount = devfs_umount,
    .sbl_head = 0
};

/* There is only one devfs, but it can be mounted multiple times */
vnode_t * vn_devfs;

void devfs_init(void) __attribute__((constructor));
void devfs_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(ramfs_init);

    const char failed[] = "Failed to init";
    int err;

    /* Root dir */
    vn_devfs = kmalloc(sizeof(vnode_t));
    if (!vn_devfs) {
        panic(failed);
    }

    vn_devfs->vn_mountpoint = vn_devfs;
    vn_devfs->vn_refcount = 1;

    err = fs_mount(vn_devfs, "", "ramfs", 0, "", 1);
    if (err) {
        char buf[80];

        ksprintf(buf, sizeof(buf), "%s : %i\n", failed, err);
        KERROR(KERROR_ERR, buf);
        return;
    }
    vn_devfs = vn_devfs->vn_mountpoint;
    kfree(vn_devfs->vn_prev_mountpoint);
    vn_devfs->vn_prev_mountpoint = vn_devfs;
    vn_devfs->vn_mountpoint = vn_devfs;

    vn_devfs->sb->vdev_id = DEV_MMTODEV(DEVFS_VDEV_MAJOR_ID, 0);
    fs_register(&devfs_fs);

    SUBSYS_INITFINI("devfs OK");
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
    /* TODO */
    return -EBUSY;
}

int dev_make(struct dev_info * devnfo, uid_t uid, gid_t gid, int perms)
{
    vnode_t * result;
    int retval;

    /* TODO Auto-numbering feature */

    if (!vn_devfs->vnode_ops->lookup(vn_devfs, devnfo->dev_name,
            sizeof(devnfo->dev_name), NULL)) {
        return -EEXIST;
    }
    retval = vn_devfs->vnode_ops->mknod(vn_devfs, devnfo->dev_name,
            sizeof(devnfo->dev_name), S_IFCHR, devnfo, &result);
    if (retval)
        return retval;

    /* Replace ops with our own */
    result->vnode_ops = (struct vnode_ops *)(&devfs_vnode_ops);

    return 0;
}

void dev_destroy(struct dev_info * devnfo)
{
}

ssize_t dev_read(vnode_t * vnode, const off_t * offset,
        void * vbuf, size_t count)
{
    struct dev_info * devnfo = (struct dev_info *)vnode->vn_specinfo;
    uint8_t * buf = (uint8_t *)vbuf;

    if (!devnfo->read)
        return -EOPNOTSUPP;

    if ((devnfo->flags & DEV_FLAGS_MB_READ) &&
            ((count / devnfo->block_size) > 1)) {
        return devnfo->read(devnfo, *offset, buf, count);
    }

    size_t buf_offset = 0;
    off_t block_offset = 0;
    do {
        int tries = RW_MAX_TRIES;
        size_t to_read = (count > devnfo->block_size) ?
            devnfo->block_size : count;

        while (1) {
            int ret = devnfo->read(devnfo, *offset + block_offset,
                                   &buf[buf_offset], to_read);
            if (ret < 0) {
                tries--;
                if (tries <= 0)
                    return (buf_offset > 0) ? buf_offset : ret;
            } else {
                break;
            }
        }

        buf_offset += to_read;
        block_offset++;

        if (count < devnfo->block_size)
            count = 0;
        else
            count -= devnfo->block_size;
    } while (count > 0);

    return buf_offset;
}

ssize_t dev_write(vnode_t * vnode, const off_t * offset,
        const void * vbuf, size_t count)
{
    struct dev_info * devnfo = (struct dev_info *)vnode->vn_specinfo;
    uint8_t * buf = (uint8_t *)vbuf;

    if (!devnfo->write)
        return -EOPNOTSUPP;

    if ((devnfo->flags & DEV_FLAGS_MB_WRITE) &&
            ((count / devnfo->block_size) > 1)) {
        return devnfo->write(devnfo, *offset, buf, count);
    }

    size_t buf_offset = 0;
    off_t block_offset = 0;
    do {
        int tries = RW_MAX_TRIES;
        size_t to_write = (count > devnfo->block_size) ?
            devnfo->block_size : count;

        while (1) {
            int ret = devnfo->write(devnfo, *offset + block_offset,
                                    &buf[buf_offset], to_write);
            if (ret < 0) {
                tries--;
                if(tries <= 0)
                    return (buf_offset > 0) ? buf_offset : ret;
            } else {
                break;
            }
        }

        buf_offset += to_write;
        block_offset++;

        if (count < devnfo->block_size)
            count = 0;
        else
            count -= devnfo->block_size;
    } while (count > 0);

    return buf_offset;
}

