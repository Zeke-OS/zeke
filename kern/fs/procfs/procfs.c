/**
 *******************************************************************************
 * @file    procfs.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
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

#include <errno.h>
#include <hal/core.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <kerror.h>
#include <proc.h>
#include <fs/ramfs.h>
#include <fs/procfs.h>

static int procfs_mount(const char * source, uint32_t mode,
                       const char * parm, int parm_len,
                       struct fs_superblock ** sb);
static int procfs_umount(struct fs_superblock * fs_sb);
static ssize_t procfs_read(file_t * file, void * vbuf, size_t bcount);
static ssize_t procfs_write(file_t * file, const void * vbuf, size_t bcount);
static int procfs_updatedir(vnode_t * dir);
static int create_status_file(vnode_t * dir, const proc_info_t * proc);


static const vnode_ops_t * procfs_vnode_ops;

static fs_t procfs_fs = {
    .fsname = PROCFS_FSNAME,
    .mount = procfs_mount,
    .umount = procfs_umount,
    .sbl_head = 0
};

/* There is only one procfs, but it can be mounted multiple times */
static vnode_t * vn_procfs;

#define PANIC_MSG "procfs_init(): "

int procfs_init(void) __attribute__((constructor));
int procfs_init(void)
{
    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("procfs");

    vnode_ops_t * vnops;

    vnops = kmalloc(sizeof(vnode_ops_t));
    if (!vnops) {
        panic(PANIC_MSG "ENOMEM\n");
    }

    /*
     * Create a vnops struct.
     * We want to inherit ops from ramfs and change pointers to overridden
     * functions.
     */
    memcpy(vnops, &ramfs_vnode_ops, sizeof(vnode_ops_t));
    vnops->read = procfs_read;
    vnops->write = procfs_write;
    procfs_vnode_ops = vnops;

    vn_procfs = fs_create_pseudofs_root(PROCFS_FSNAME, PROCFS_MAJOR_NUM);
    fs_register(&procfs_fs);
    procfs_updatedir(vn_procfs);

    return 0;
}

static int procfs_mount(const char * source, uint32_t mode,
                       const char * parm, int parm_len,
                       struct fs_superblock ** sb)
{
    *sb = vn_procfs->sb;
    return 0;
}

static int procfs_umount(struct fs_superblock * fs_sb)
{
    /* TODO */
    return -EBUSY;
}

static ssize_t procfs_read(file_t * file, void * vbuf, size_t bcount)
{
    size_t bbytes;
    ssize_t bytes = 0;
    struct procfs_info * spec = (struct procfs_info *)file->vnode->vn_specinfo;
    proc_info_t * proc;
    char * tmpbuf;

    if (!spec)
        return -EIO;

    tmpbuf = kcalloc(200, sizeof(char));
    if (!tmpbuf)
        return -EIO;

    if (spec->ftype == PROCFS_STATUS) {
        PROC_LOCK();
        proc = proc_get_struct(spec->pid);
        if (!proc) {
            PROC_UNLOCK();
            return -ENOLINK;
        }
        PROC_UNLOCK();

        bbytes = ksprintf(tmpbuf, bcount,
                "Name: %s\n"
                "State: %u\n"
                "Pid: %u\n"
                "Uid: %u %u %u\n"
                "Gid: %u %u %u\n",
                proc->name,
                proc->state,
                proc->pid,
                proc->uid, proc->euid, proc->suid,
                proc->gid, proc->egid, proc->sgid);

        if (file->seek_pos < bbytes) {
            bytes = strlcpy((char *)vbuf, tmpbuf + file->seek_pos,
                            min(bcount, bbytes));
            file->seek_pos = min(file->seek_pos + bytes, bbytes);
        }

        kfree(tmpbuf);
    }

    return bytes;
}

static ssize_t procfs_write(file_t * file, const void * vbuf, size_t bcount)
{
    return -EROFS; /* TODO is there any files that should allow writing? */
}

/* TODO Should be called automatically! */
static int procfs_updatedir(vnode_t * dir)
{
    int err = 0;

    PROC_LOCK();

    if (dir == vn_procfs) {
        /*
         * This is the procfs root.
         *
         * Now we just try to create a directory for every process in the system
         * regardless whether it already exist or not and try to remove
         * directories that should not exist anymore.
         */
        for (int i = 0; i < act_maxproc; i++) {
            vnode_t * pdir;
            const proc_info_t * proc = proc_get_struct(i);
            char name[10];
            size_t name_len;

            if (!proc)
                continue;

            /* proc dir name and name_len */
            name_len = uitoa32(name, proc->pid);

            dir->vnode_ops->mkdir(dir, name, name_len, 0444);
            err = dir->vnode_ops->lookup(dir, name, name_len, &pdir);
            if (err) {
                err = -ENOTDIR;
                goto fail;
            }

            err = create_status_file(pdir, proc);
            if (err) {
                err = -ENOTDIR;
                goto fail;
            }
        }

        /* TODO Remove old process dirs */
    }

fail:
    PROC_UNLOCK();

    return err;
}

static int create_status_file(vnode_t * dir, const proc_info_t * proc)
{
    vnode_t * vn;
    const char name[7] = "status";
    const size_t name_len = 7;
    struct procfs_info * spec = kcalloc(1, sizeof(struct procfs_info));
    int err;

    if (!spec) {
        PROC_UNLOCK();
        return -ENOTDIR;
    }

    /* Create a specinfo */
    spec->ftype = PROCFS_STATUS;
    spec->pid = proc->pid;

    err = dir->vnode_ops->mknod(dir, name, name_len, S_IFREG | 0444, spec, &vn);
    if (err) {
        kfree(spec);
        return -ENOTDIR;
    }

    vn->vn_specinfo = spec;
    vn->vnode_ops = (struct vnode_ops *)(procfs_vnode_ops);

    return 0;
}