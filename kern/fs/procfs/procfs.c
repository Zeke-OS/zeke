/**
 *******************************************************************************
 * @file    procfs.c
 * @author  Olli Vanhoja
 * @brief   Process file system.
 * @section LICENSE
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

static int procfs_mount(fs_t * fs, const char * source, uint32_t mode,
                        const char * parm, int parm_len,
                        struct fs_superblock ** sb);
static int procfs_umount(struct fs_superblock * fs_sb);
static ssize_t procfs_read(file_t * file, struct uio * uio, size_t bcount);
static ssize_t procfs_write(file_t * file, struct uio * uio, size_t bcount);
static void procfs_event_fd_created(struct proc_info * p, file_t * file);
static void procfs_event_fd_closed(struct proc_info * p, file_t * file);
static int procfs_delete_vnode(vnode_t * vnode);
static int create_proc_file(vnode_t * pdir, pid_t pid, const char * filename,
                            enum procfs_filetype ftype);


static vnode_ops_t procfs_vnode_ops = {
    .read = procfs_read,
    .write = procfs_write,
    .event_fd_created = procfs_event_fd_created,
    .event_fd_closed = procfs_event_fd_closed,
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
static procfs_relefn_t ** procfs_rele_funcs;

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
    procfs_rele_funcs = kcalloc(SET_COUNT(procfs_files),
            sizeof(procfs_relefn_t *));

    if (!(procfs_read_funcs && procfs_write_funcs && procfs_rele_funcs))
        return -ENOMEM;

    SET_FOREACH(file, procfs_files) {
        enum procfs_filetype filetype = (*file)->filetype;

        procfs_read_funcs[filetype] = (*file)->readfn;
        procfs_write_funcs[filetype] = (*file)->writefn;
        procfs_rele_funcs[filetype] = (*file)->relefn;

        const char * filename = (*file)->filename;
        create_proc_file(vn_procfs, 0, filename, filetype);
    }

    return 0;
}

int __kinit__ procfs_init(void)
{
    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("procfs");

    /*
     * This must be static as it's referenced and used in the file system via
     * the fs object system.
     */
    static fs_t procfs_fs = {
        .fsname = PROCFS_FSNAME,
        .mount = procfs_mount,
        .sblist_head = SLIST_HEAD_INITIALIZER(),
    };

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

    return 0;
}

static int procfs_mount(fs_t * fs, const char * source, uint32_t mode,
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
    const struct procfs_info * spec = PROCFS_GET_FILESPEC(file);
    procfs_relefn_t * fn;

    if (!spec || spec->ftype > PROCFS_LAST)
        return;

    fn = procfs_rele_funcs[spec->ftype];
    if (!fn)
        return;

    fn(file->stream);
}

void procfs_kfree_stream(struct procfs_stream * stream)
{
    kfree(stream);
}

static int procfs_delete_vnode(vnode_t * vnode)
{
    const struct procfs_info * spec = vnode->vn_specinfo;

    if (!spec || spec->ftype <= PROCFS_LAST)
        mempool_return(specinfo_pool, vnode->vn_specinfo);
    return ramfs_delete_vnode(vnode);
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

    spec->vnode = vn;
    vn->vn_specinfo = spec;
    vn->vnode_ops = &procfs_vnode_ops;

    vrele(vn);
    return 0;
}
