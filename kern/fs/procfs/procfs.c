/**
 *******************************************************************************
 * @file    procfs.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
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
#include <hal/core.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <kerror.h>
#include <proc.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/ramfs.h>
#include <fs/dev_major.h>
#include <fs/procfs.h>

static int procfs_mount(const char * source, uint32_t mode,
                        const char * parm, int parm_len,
                        struct fs_superblock ** sb);
static int procfs_umount(struct fs_superblock * fs_sb);
static ssize_t procfs_read(file_t * file, void * vbuf, size_t bcount);
static ssize_t procfs_write(file_t * file, const void * vbuf, size_t bcount);
static int procfs_updatedir(vnode_t * dir);
static int create_status_file(vnode_t * dir, const proc_info_t * proc);


static vnode_ops_t procfs_vnode_ops = {
    .read = procfs_read,
    .write = procfs_write,
};

static fs_t procfs_fs = {
    .fsname = PROCFS_FSNAME,
    .mount = procfs_mount,
    .umount = procfs_umount,
    .sblist_head = SLIST_HEAD_INITIALIZER(),
};

/* There is only one procfs, but it can be mounted multiple times */
static vnode_t * vn_procfs;

#define PANIC_MSG "procfs_init(): "

int procfs_init(void) __attribute__((constructor));
int procfs_init(void)
{
    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("procfs");

    /*
     * Inherit unimplemented vnops from ramfs.
     */
    fs_inherit_vnops(&procfs_vnode_ops, &ramfs_vnode_ops);

    vn_procfs = fs_create_pseudofs_root(&procfs_fs, VDEV_MJNR_PROCFS);
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
    /* TODO implementation of procfs_umount() */
    return -EBUSY;
}

/**
 * Override read() function.
 */
static ssize_t procfs_read(file_t * file, void * vbuf, size_t bcount)
{
    size_t bbytes;
    ssize_t bytes = 0;
    struct procfs_info * spec = (struct procfs_info *)file->vnode->vn_specinfo;
    proc_info_t * proc;
    char * tmpbuf;
    const size_t bufsz = 200;

    if (!spec)
        return -EIO;

    tmpbuf = kcalloc(bufsz, sizeof(char));
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

        bbytes = ksprintf(tmpbuf, bufsz,
                "Name: %s\n"
                "State: %u\n"
                "Pid: %u\n"
                "Uid: %u %u %u\n"
                "Gid: %u %u %u\n"
                "User: %u\n"
                "Sys: %u\n"
                "Break: %p %p\n",
                proc->name,
                proc->state,
                proc->pid,
                proc->uid, proc->euid, proc->suid,
                proc->gid, proc->egid, proc->sgid,
                proc->tms.tms_utime, proc->tms.tms_stime,
                proc->brk_start, proc->brk_stop);

        if (file->seek_pos <= bbytes) {
            const size_t count = min(bcount, bbytes - file->seek_pos);

            if (count <= bbytes) {
                size_t sz;

                sz = strlcpy((char *)vbuf, tmpbuf + file->seek_pos, count);
                sz++;
                if (sz >= count)
                    bytes = count;
                else
                    bytes = sz;
                file->seek_pos += bytes;
            }
        }

        kfree(tmpbuf);
    }

    return bytes;
}

/**
 * Override write() function.
 */
static ssize_t procfs_write(file_t * file, const void * vbuf, size_t bcount)
{
    return -EROFS; /* TODO is there any files that should allow writing? */
}

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
        for (int i = 0; i <= act_maxproc; i++) {
            const proc_info_t * proc = proc_get_struct(i);

            if (proc)
                err = procfs_mkentry(proc);
            else
                procfs_rmentry(i);
        }
    }

    PROC_UNLOCK();

    return err;
}

int procfs_mkentry(const proc_info_t * proc)
{
#ifdef configPROCFS_DEBUG
    const char fail[] = "Failed to create a procfs entry\n";
#endif
    char name[PROCFS_NAMELEN_MAX];
    vnode_t * pdir = NULL;
    int err;

    if (!vn_procfs)
        return 0; /* Not yet initialized. */

    uitoa32(name, proc->pid);

#ifdef configPROCFS_DEBUG
    KERROR(KERROR_DEBUG, "procfs_mkentry(pid = %s)\n", name);
#endif

    err = vn_procfs->vnode_ops->mkdir(vn_procfs, name, PROCFS_PERMS);
    if (err == -EEXIST) {
        return 0;
    } else if (err) {
#ifdef configPROCFS_DEBUG
        KERROR(KERROR_DEBUG, fail);
#endif
        goto fail;
    }

    err = vn_procfs->vnode_ops->lookup(vn_procfs, name, &pdir);
    if (err) {
#ifdef configPROCFS_DEBUG
        KERROR(KERROR_DEBUG, fail);
#endif
        pdir = NULL;
        goto fail;
    }

    err = create_status_file(pdir, proc);
    if (err) {
#ifdef configPROCFS_DEBUG
        KERROR(KERROR_DEBUG, fail);
#endif
        goto fail;
    }

fail:
    if (pdir)
        vrele(pdir);
    return err;
}

void procfs_rmentry(pid_t pid)
{
    vnode_t * pdir;
    char name[PROCFS_NAMELEN_MAX];
    int err;

    if (!vn_procfs)
        return; /* Not yet initialized. */

    uitoa32(name, pid);

#ifdef configPROCFS_DEBUG
        KERROR(KERROR_DEBUG, "procfs_rmentry(pid = %s)\n", name);
#endif

    vref(vn_procfs);

    err = vn_procfs->vnode_ops->lookup(vn_procfs, name, &pdir);
    if (err) {
#ifdef configPROCFS_DEBUG
        KERROR(KERROR_DEBUG, "pid dir doesn't exist\n");
#endif
        vrele(vn_procfs);
        return;
    }

    pdir->vnode_ops->unlink(pdir, PROCFS_STATUS_FILE);
    vrele(pdir);
    err = vn_procfs->vnode_ops->rmdir(vn_procfs, name);
#ifdef configPROCFS_DEBUG
    if (err)
        KERROR(KERROR_DEBUG, "Can't rmdir(%s)\n", name);
#endif

    vrele(vn_procfs);
}

/**
 * Create a process status file.
 */
static int create_status_file(vnode_t * pdir, const proc_info_t * proc)
{
    vnode_t * vn;
    struct procfs_info * spec;
    int err;

    KASSERT(pdir != NULL, "pdir must be set");

    spec = kcalloc(1, sizeof(struct procfs_info));
    if (!spec)
        return -ENOMEM;

    /* Create a specinfo */
    spec->ftype = PROCFS_STATUS;
    spec->pid = proc->pid;

    err = pdir->vnode_ops->mknod(pdir, PROCFS_STATUS_FILE,
            S_IFREG | PROCFS_PERMS, spec, &vn);
    if (err) {
        kfree(spec);
        return -ENOTDIR;
    }

    vn->vn_specinfo = spec;
    vn->vnode_ops = &procfs_vnode_ops;

    vrele(vn);

    return 0;
}
