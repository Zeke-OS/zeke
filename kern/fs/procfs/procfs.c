/**
 *******************************************************************************
 * @file    procfs.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stddef.h>
#include <sys/dev_major.h>
#include <errno.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <fs/procfs.h>
#include <fs/ramfs.h>
#include <hal/core.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <mempool.h>
#include <proc.h>

#define PROCFS_GET_FILESPEC(_file_) \
    ((struct procfs_info *)(_file_)->vnode->vn_specinfo)

static int procfs_mount(const char * source, uint32_t mode,
                        const char * parm, int parm_len,
                        struct fs_superblock ** sb);
static int procfs_umount(struct fs_superblock * fs_sb);
static ssize_t procfs_read(file_t * file, struct uio * uio, size_t bcount);
static ssize_t procfs_write(file_t * file, struct uio * uio, size_t bcount);
static void procfs_event_fd_created(struct proc_info * p, file_t * file);
static void procfs_event_fd_closed(struct proc_info * p, file_t * file);
static int procfs_delete_vnode(vnode_t * vnode);
static int procfs_updatedir(vnode_t * dir);
static int create_proc_file(vnode_t * pdir, pid_t pid, const char * filename,
                            enum procfs_filetype ftype);


static vnode_ops_t procfs_vnode_ops = {
    .read = procfs_read,
    .write = procfs_write,
    .event_fd_created = procfs_event_fd_created,
    .event_fd_closed = procfs_event_fd_closed,
};

static fs_t procfs_fs = {
    .fsname = PROCFS_FSNAME,
    .mount = procfs_mount,
    .sblist_head = SLIST_HEAD_INITIALIZER(),
};

/**
 * Procfs root vnode.
 * There is only one procfs, but it can be mounted multiple times.
 */
static vnode_t * vn_procfs;

static struct mempool * specinfo_pool;

SET_DECLARE(procfs_files, struct procfs_file);
static procfs_readfn_t ** procfs_read_funcs;
static procfs_writefn_t ** procfs_write_funcs;

/**
 * Initialize permanently existing procfs files.
 */
static int init_permanent_files(void)
{
    struct procfs_file ** file;

    procfs_read_funcs = kcalloc(SET_COUNT(procfs_files),
            sizeof(procfs_readfn_t *));
    procfs_write_funcs = kcalloc(SET_COUNT(procfs_files),
            sizeof(procfs_writefn_t *));

    if (!(procfs_read_funcs && procfs_write_funcs))
        return -ENOMEM;

    SET_FOREACH(file, procfs_files) {
        enum procfs_filetype filetype = (*file)->filetype;

        procfs_read_funcs[filetype] = (*file)->readfn;
        procfs_write_funcs[filetype] = (*file)->writefn;

        if (filetype > PROCFS_KERNEL_SEPARATOR) {
            const char * filename = (*file)->filename;

            create_proc_file(vn_procfs, 0, filename, filetype);
        }
    }

    return 0;
}

int __kinit__ procfs_init(void)
{
    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("procfs");

    specinfo_pool = mempool_init(MEMPOOL_TYPE_NONBLOCKING,
                                 sizeof(struct procfs_info),
                                 configMAXPROC);
    if (!specinfo_pool)
        return -ENOMEM;

    FS_GIANT_INIT(&procfs_fs.fs_giant);

    /*
     * Inherit unimplemented vnops from ramfs.
     */
    fs_inherit_vnops(&procfs_vnode_ops, &ramfs_vnode_ops);

    vn_procfs = fs_create_pseudofs_root(&procfs_fs, VDEV_MJNR_PROCFS);
    if (!vn_procfs)
        return -ENOMEM;

    struct fs_superblock * sb = vn_procfs->sb;
    sb->delete_vnode = procfs_delete_vnode;

    vn_procfs->sb->umount = procfs_umount;
    fs_register(&procfs_fs);

    int err = init_permanent_files();
    if (err)
        return err;

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
    /* NOP, everything relevant is handled by the vfs. */

    return 0;
}

/**
 * Override read() function.
 */
static ssize_t procfs_read(file_t * file, struct uio * uio, size_t bcount)
{
    const struct procfs_info * spec = PROCFS_GET_FILESPEC(file);
    struct procfs_stream * stream;
    void * vbuf;
    ssize_t bytes;
    int err;

    if (!spec || spec->ftype > PROCFS_LAST || !file->stream)
        return -EIO;

    err = uio_get_kaddr(uio, &vbuf);
    if (err)
        return err;

    stream = (struct procfs_stream *)file->stream;
    bytes = stream->bytes;
    if (bytes > 0 && file->seek_pos <= bytes) {
        const ssize_t count = min(bcount, bytes - file->seek_pos);

        if (count <= bytes) {
            ssize_t sz;

            sz = strlcpy((char *)vbuf, stream->buf + file->seek_pos, count);
            sz++;
            bytes = (sz >= count) ? count : sz;
            file->seek_pos += bytes;
        }
    }

    return bytes;
}

/**
 * Override write() function.
 */
static ssize_t procfs_write(file_t * file, struct uio * uio, size_t bcount)
{
    const struct procfs_info * spec = PROCFS_GET_FILESPEC(file);
    procfs_writefn_t * fn;
    void * vbuf;
    int err;

    if (!spec)
        return -EIO;

    if (spec->ftype > PROCFS_LAST)
        return -ENOLINK;

    fn = procfs_write_funcs[spec->ftype];
    if (!fn)
        return -ENOTSUP;

    err = uio_get_kaddr(uio, &vbuf);
    if (err)
        return err;

    return fn(spec, (struct procfs_stream *)(file->stream),
              vbuf, bcount);
}

static void procfs_event_fd_created(struct proc_info * p, file_t * file)
{
    const struct procfs_info * spec = PROCFS_GET_FILESPEC(file);
    procfs_readfn_t * fn;

    if (!spec || spec->ftype > PROCFS_LAST)
        return;

    fn = procfs_read_funcs[spec->ftype];
    if (!fn)
        return;

    file->stream = fn(spec);
}

static void procfs_event_fd_closed(struct proc_info * p, file_t * file)
{
    kfree(file->stream);
}

static int procfs_delete_vnode(vnode_t * vnode)
{
    const struct procfs_info * spec = vnode->vn_specinfo;

    if (!spec || spec->ftype <= PROCFS_LAST)
        mempool_return(specinfo_pool, vnode->vn_specinfo);
    return ramfs_delete_vnode(vnode);
}

static int procfs_updatedir(vnode_t * dir)
{
    int err = 0;

    if (dir == vn_procfs) {
        PROC_LOCK();

        /*
         * This is the procfs root.
         *
         * Now we just try to create a directory for every process in the system
         * regardless whether it already exist or not and try to remove
         * directories that should not exist anymore.
         */
        for (int i = 0; i <= configMAXPROC; i++) {
            struct proc_info * proc = proc_ref(i, PROC_LOCKED);

            if (proc) {
                err = procfs_mkentry(proc);
                proc_unref(proc);
            } else {
                procfs_rmentry(i);
            }
        }

        PROC_UNLOCK();
    }

    return err;
}

int procfs_mkentry(const struct proc_info * proc)
{
    char name[PROCFS_NAMELEN_MAX];
    vnode_t * pdir = NULL;
    struct procfs_file ** file;
    int err;

    if (!vn_procfs)
        return 0; /* Not yet initialized. */

    uitoa32(name, proc->pid);

    KERROR_DBG("%s(pid = %s)\n", __func__, name);

    err = vn_procfs->vnode_ops->mkdir(vn_procfs, name, PROCFS_PERMS);
    if (err == -EEXIST) {
        return 0;
    } else if (err) {
        goto fail;
    }

    err = vn_procfs->vnode_ops->lookup(vn_procfs, name, &pdir);
    if (err) {
        pdir = NULL;
        goto fail;
    }

    SET_FOREACH(file, procfs_files) {
        const enum procfs_filetype filetype = (*file)->filetype;

        if (filetype < PROCFS_KERNEL_SEPARATOR) {
            const char * filename = (*file)->filename;

            err = create_proc_file(pdir, proc->pid, filename, filetype);
            if (err)
                goto fail;
        }
    }

fail:
    if (pdir)
        vrele(pdir);
    if (err)
        KERROR_DBG("Failed to create a procfs entry\n");
    return err;
}

void procfs_rmentry(pid_t pid)
{
    vnode_t * pdir;
    char name[PROCFS_NAMELEN_MAX];
    struct procfs_file ** file;

    if (!vn_procfs)
        return; /* Not yet initialized. */

    uitoa32(name, pid);

    KERROR_DBG("%s(pid = %s)\n", __func__, name);

    vref(vn_procfs);

    if (vn_procfs->vnode_ops->lookup(vn_procfs, name, &pdir)) {
        KERROR_DBG("pid dir doesn't exist\n");
        goto out;
    }

    SET_FOREACH(file, procfs_files) {
        if ((*file)->filetype < PROCFS_KERNEL_SEPARATOR) {
            pdir->vnode_ops->unlink(pdir, (*file)->filename);
        }
    }


    vrele(pdir);
    if (vn_procfs->vnode_ops->rmdir(vn_procfs, name)) {
        KERROR_DBG("Can't rmdir(%s)\n", name);
    }

out:
    vrele(vn_procfs);
}

/**
 * Create a process specific file.
 */
static int create_proc_file(vnode_t * pdir, pid_t pid, const char * filename,
                            enum procfs_filetype ftype)
{
    vnode_t * vn;
    struct procfs_info * spec;
    int err;

    KASSERT(pdir != NULL, "pdir must be set");

    spec = mempool_get(specinfo_pool);
    if (!spec)
        return -ENOMEM;

    /* Create a specinfo */
    spec->ftype = ftype;
    spec->pid = pid;

    err = pdir->vnode_ops->mknod(pdir, filename, S_IFREG | PROCFS_PERMS, spec,
                                 &vn);
    if (err) {
        mempool_return(specinfo_pool, spec);
        return -ENOTDIR;
    }

    vn->vn_specinfo = spec;
    vn->vnode_ops = &procfs_vnode_ops;

    vrele(vn);
    return 0;
}
