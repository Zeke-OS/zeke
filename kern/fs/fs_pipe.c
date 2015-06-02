/**
 *******************************************************************************
 * @file    fs_pipe.c
 * @author  Olli Vanhoja
 * @brief   IPC pipes.
 * @section LICENSE
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <buf.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <libkern.h>
#include <proc.h>
#include <queue_r.h>
#include <kern_ipc.h>

/*
 * TODO
 * - We should guarantee atomicity for transactions uder PIPE_BUF bytes
 * - Setting O_ASYNC should cause SIGIO to be sent if new input becomes
 *   available
 */

/*
 * Just a humble guess that lazy yield may render improved performance on
 * MP system.
 */
#ifdef configMP
#define PIPE_YIELD_STRATEGY THREAD_YIELD_LAZY
#else
#define PIPE_YIELD_STRATEGY THREAD_YIELD_IMMEDIATE
#endif

/**
 * Pipe descriptor pointed by file->stream.
 */
struct stream_pipe {
    struct vnode vnode;
    struct queue_cb q;
    struct buf * bp;
    file_t file0; /*!< Read end. */
    file_t file1; /*!< Write end. */
    uid_t owner;
    gid_t group;
};

static ssize_t fs_pipe_write(file_t * file, const void * buf, size_t count);
static ssize_t fs_pipe_read(file_t * file, void * buf, size_t count);
static int fs_pipe_stat(vnode_t * vnode, struct stat * stat);
static int fs_pipe_chmod(vnode_t * vnode, mode_t mode);
static int fs_pipe_chown(vnode_t * vnode, uid_t owner, gid_t group);
static int fs_pipe_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                             vnode_t ** vnode);

static vnode_ops_t fs_pipe_ops = {
    .write = fs_pipe_write,
    .read = fs_pipe_read,
    .stat = fs_pipe_stat,
    .chmod = fs_pipe_chmod,
    .chown = fs_pipe_chown,
};

static struct fs fs_pipe_fs = {
    .fsname = "pipefs",
    .mount = NULL,
    .sblist_head = SLIST_HEAD_INITIALIZER(),
};

static struct fs_superblock fs_pipe_sb = {
    .fs = &fs_pipe_fs,
    .get_vnode = fs_pipe_get_vnode,
    .delete_vnode = fs_pipe_destroy,
    .umount = NULL,
};

int __kinit__ fs_pipe_init(void)
{
    SUBSYS_DEP(ramfs_init);
    SUBSYS_INIT("fs_pipe");

    FS_GIANT_INIT(&fs_pipe_fs.fs_giant);
    fs_inherit_vnops(&fs_pipe_ops, &nofs_vnode_ops);

    return 0;
}

static void init_file(file_t * file, vnode_t * vn, struct stream_pipe * pipe,
                      int oflags)
{
    fs_fildes_set(file, vn, oflags);

    file->fdflags &= ~FD_CLOEXEC;
    file->stream = pipe;
}

/**
 * @param files is the target files struct.
 * @param fildes returned file descriptor numbers.
 * @param len is the minimum size of the new pipe buffer.
 */
int fs_pipe_curproc_creat(struct files_struct * files, int fildes[2],
                          size_t len)
{
    struct stream_pipe * pipe;
    file_t * file0;
    file_t * file1;
    vnode_t * vnode;
    struct buf * bp;

    len = memalign_size(len, MMU_PGSIZE_COARSE);

    /*
     * Allocate space for structs and get a buffer.
     *
     * +------------+
     * | pipe       |<--.
     * +------------+   |
     * | bp         |----------.
     * | q          |   |      |
     * |  data      |-------------------.
     * | file0      |   |      |        |
     * |  stream    |---+      |        |
     * |  ...       |   |      |        |
     * | file1      |   |      |        |
     * |  stream    |---|      |        |
     * |  ...       |   |      |        |
     * | vnode      |   |      |        |
     * |  specinfo  |---       |        |
     * | owner      |          \/       |
     * | group      |      +--------+   |
     * +------------+      | buf    |   |
     *                     +--------+   |
     *                     | b_data |---+
     *                     +--------+   |
     *                                  |
     *                                  \/
     *                               +-------+
     *                               |       |
     */
    pipe = kzalloc(sizeof(struct stream_pipe));
    bp = geteblk(len);
    if (!(pipe && bp)) {
        kfree(pipe);
        if (bp)
            bp->vm_ops->rfree(bp);

        return -ENOMEM;
    }
    file0 = &pipe->file0;
    file1 = &pipe->file1;
    vnode = &pipe->vnode;

    /* Init queue */
    pipe->bp = bp;
    pipe->q = queue_create((char *)bp->b_data, sizeof(char), len);
    pipe->owner = curproc->cred.euid;
    pipe->group = curproc->cred.egid;

    /* Init vnode */
    fs_vnode_init(vnode, 0, &fs_pipe_sb, &fs_pipe_ops);
    vrefset(vnode, 2); /* Two file descripts by default. */
    vnode->vn_len = len;
    vnode->vn_mode = S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    vnode->vn_specinfo = pipe;

    /* Init file descriptors */
    init_file(file0, vnode, pipe, O_RDONLY);
    init_file(file1, vnode, pipe, O_WRONLY);

    /* ... and we are ready */
    fildes[0] = fs_fildes_curproc_next(file0, 0);
    fildes[1] = fs_fildes_curproc_next(file1, 0);

    return 0;
}

/*
 * This is called when vnode refcount <= 0.
 */
int fs_pipe_destroy(vnode_t * vnode)
{
    struct stream_pipe * pipe = (struct stream_pipe *)vnode->vn_specinfo;
    struct buf * bp = pipe->bp;

    bp->vm_ops->rfree(bp);
    kfree(pipe);

    return 0;
}

ssize_t fs_pipe_write(file_t * file, const void * buf, size_t count)
{
    struct stream_pipe * pipe = (struct stream_pipe *)file->stream;

    if (!(file->oflags & O_WRONLY))
        return -EBADF;

    if (atomic_read(&pipe->file0.refcount) < 1) {
        return -EPIPE;
    }

    /* TODO Implement O_NONBLOCK */

    for (size_t i = 0; i < count;) {
        if (queue_push(&pipe->q, (char *)buf + i))
            i++;
        thread_yield(PIPE_YIELD_STRATEGY);
    }

    return count;
}

ssize_t fs_pipe_read(file_t * file, void * buf, size_t count)
{
    struct stream_pipe * pipe = (struct stream_pipe *)file->stream;
    int trycount = 0;

    /*
     * TODO Atomic pipes
     *
     * Instead of using that trycount we should actually make pipes atomic per
     * PIPE_BUF and return immediately after some data is received.
     */

    if (!(file->oflags & O_RDONLY))
        return -EBADF;

    /* TODO Implement O_NONBLOCK */

    for (size_t i = 0; i < count;) {
        if (queue_isempty(&pipe->q) &&
                ((trycount++ > 5 && i > 0) ||
                 atomic_read(&pipe->file1.refcount) < 1)) {
            return i;
        }

        if (queue_pop(&pipe->q, (char *)buf + i))
            i++;
        thread_yield(PIPE_YIELD_STRATEGY);
    }

    return count;
}

int fs_pipe_stat(vnode_t * vnode, struct stat * stat)
{
    struct stream_pipe * pipe = (struct stream_pipe *)vnode->vn_specinfo;

    stat->st_dev = 0;
    stat->st_ino = vnode->vn_num;
    stat->st_mode = vnode->vn_mode;
    stat->st_nlink = vnode->vn_refcount;
    stat->st_uid = pipe->owner;
    stat->st_gid = pipe->group;
    stat->st_rdev = 0;
    stat->st_size = pipe->bp->b_bcount;
#if 0
    stat->st_atime; /* TODO times */
    stat->st_mtime;
    stat->st_ctime;
    stat->st_birthtime;
#endif
    stat->st_flags = 0;
    stat->st_blksize = sizeof(char);
    stat->st_blocks = stat->st_size;

    return 0;
}

int fs_pipe_chmod(vnode_t * vnode, mode_t mode)
{
    vnode->vn_mode = mode;

    return 0;
}

int fs_pipe_chown(vnode_t * vnode, uid_t owner, gid_t group)
{
    struct stream_pipe * pipe = (struct stream_pipe *)vnode->vn_specinfo;

    pipe->owner = owner;
    pipe->group = group;

    return 0;
}

static int fs_pipe_get_vnode(struct fs_superblock * sb, ino_t * vnode_num,
                             vnode_t ** vnode)
{
    return -ENOTSUP;
}
