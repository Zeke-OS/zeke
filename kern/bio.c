/**
 *******************************************************************************
 * @file    bio.c
 * @author  Olli Vanhoja
 * @brief   IO Buffer Cache.
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

#define KERNEL_INTERNAL
#include <sys/linker_set.h>
#include <errno.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <dllist.h>
#include <fs/devfs.h>
#include <buf.h>

#define BUFHSZ 64
#define BUFHASH(dev, bn) (((dev) + bn) & (BUFHSZ - 1))

struct bufhd {
    llist_t * li;
};

mtx_t cache_lock; /* Protect caching data structures */
struct bufhd (*bufhash)[];

static int _biowait(struct buf * bp, long timeout);

/* Init bio, called by vralloc_init() */
void _bio_init(void)
{
    bufhash = kcalloc(BUFHSZ, sizeof(struct bufhd));
    if (!bufhash)
        panic("OOM during _bio_init()");

    mtx_init(&cache_lock, MTX_TYPE_SPIN | MTX_TYPE_TICKET | MTX_TYPE_SLEEP);
}

int bread(vnode_t * vnode, size_t blkno, int size, struct buf ** bpp)
{
    struct buf * bp;

    bp = getblk(vnode, blkno, size, 0);
    if (!bp)
        return -ENOMEM;

    if (bp->b_flags & B_DONE)
        goto out;

    biowait(bp);
    bp->b_bcount = size;

out:
    *bpp = bp;
    return 0;
}

int  breadn(vnode_t * vnode, size_t blkno, int size, size_t rablks[],
            int rasizes[], int nrablks, struct buf ** bpp)
{
}

int bwrite(struct buf * bp)
{
    unsigned flags;
    file_t * file;
    mtx_t * lock;

#if configDEBUG >= KERROR_DEBUG
    if (!bp)
        panic("bp not set");
#endif

    lock = &bp->lock;
    file = &bp->b_file;

    /* Sanity check */
    if(!(bp->b_file.vnode && bp->b_file.vnode->vnode_ops &&
                bp->b_file.vnode->vnode_ops->write)) {
        mtx_spinlock(lock);
        bp->b_flags |= B_ERROR;
        bp->b_error |= -EIO;
        mtx_unlock(lock);

        return -EIO;
    }

    mtx_spinlock(lock);
    flags = bp->b_flags;
    bp->b_flags &= ~(B_DONE | B_ERROR | B_ASYNC | B_IDLWRI | B_DELWRI);
    bp->b_flags |= B_BUSY;
    mtx_unlock(lock);

    /* TODO Use dirty offsets */
    if (flags & B_ASYNC) {
        biowait(bp);

        return 0;
    } else {
        mtx_spinlock(lock);

        file->seek_pos = bp->b_blkno;
        file->vnode->vnode_ops->write(file, (void *)bp->b_data,
                bp->b_bcount);

        bp->b_flags &= ~B_BUSY;
        mtx_unlock(lock);
    }

    return 0;
}

void bawrite(struct buf * bp)
{
}

void bdwrite(struct buf * bp)
{
}

void bio_clrbuf(struct buf * bp)
{
#if configDEBUG >= KERROR_DEBUG
    if (!bp)
        panic("bp not set");
#endif

    mtx_spinlock(&bp->lock);
    if (bp->b_flags & (B_IDLWRI | B_DELWRI)) {
        biowait(bp);
    }
    bp->b_flags &= ~B_ERROR;
    bp->b_flags |= B_BUSY;
    mtx_unlock(&bp->lock);

    memset((void *)bp->b_data, 0, bp->b_bufsize);

    mtx_spinlock(&bp->lock);
    bp->b_flags &= ~B_BUSY;
    mtx_unlock(&bp->lock);
}

static struct buf * create_blk(vnode_t * vnode, size_t blkno, size_t size,
                               int slptimeo)
{
    struct buf * bp = geteblk(size);
    struct file file;

    if (!bp)
        return NULL;

    bp->b_blkno = blkno;
    file.vnode = vnode;
    file.seek_pos = blkno;
    file.oflags = O_RDWR;
    file.refcount = 1;
    file.stream = NULL;

    bp->b_file = file;
    mtx_init(&bp->b_file.lock, MTX_TYPE_SPIN);

    return bp;
}

struct buf * getblk(vnode_t * vnode, size_t blkno, size_t size, int slptimeo)
{
    size_t hash;
    struct bufhd * bf;
    dev_t dev;
    struct buf * bp;

    if (!vnode)
        return NULL;

    if (S_ISBLK(vnode->vn_mode) || S_ISCHR(vnode->vn_mode)) {
        dev = ((struct dev_info *)vnode->vn_specinfo)->dev_id;
    } else {
        dev = vnode->sb->vdev_id;
    }

    /*
     * TODO This can be probably refactored to be a part of incore().
     *      We may want to have two versions, one that requires a cache
     *      lock and one that acquires it by itself.
     */

    hash = BUFHASH(dev, blkno);

    /* Get lock to caching */
    mtx_spinlock(&cache_lock);

    bf = &(*bufhash)[hash];

    if (bf->li == NULL) {
        /* If list is not yet created we create it */
        bf->li = dllist_create(struct buf, lnode);

        goto not_found;
    }

    /* Look for a buffer in the linked list found. */
    bp = bf->li->head;
    while (bp) {
        if ((bp->b_file.vnode == vnode) && (bp->b_blkno == blkno))
            break;

        bp = bp->lnode.next;
    }
    if (!bp) {
not_found:
        bp = create_blk(vnode, blkno, size, slptimeo);
        if (!bp)
            goto fail;

        /* Insert to the hashmap list */
        bf->li->insert_head(bf->li, bp);
    }

    /* Found */
retry:
    biowait(bp); /* Wait until I/O has completed. */

    /*
     * Wait until the buffer is released. It is possible that we don't get it
     * locked for us on first try, so we just keep trying until it's not set
     * busy by some other thread.
     */
    while (bp->b_flags & B_BUSY)
        sched_current_thread_yield(0);
    mtx_spinlock(&bp->lock);
    if (bp->b_flags & B_BUSY) {
        mtx_unlock(&bp->lock);
        goto retry;
    }
    bp->b_flags |= B_BUSY;
    mtx_unlock(&bp->lock);

    allocbuf(bp, size); /* Resize if necessary */

    mtx_spinlock(&bp->lock);
    bp->b_flags &= ~B_ERROR;
    bp->b_error = 0;
    mtx_unlock(&bp->lock);

fail:
    mtx_unlock(&cache_lock);

    return bp;
}

struct buf * incore(vnode_t * vnode, size_t blkno)
{
}

void biodone(struct buf * bp)
{
    mtx_spinlock(&bp->lock);

    bp->b_flags |= B_DONE;

    if (bp->b_flags & B_ASYNC)
        brelse(bp);

    mtx_unlock(&bp->lock);
}

static int _biowait(struct buf * bp, long timeout)
{
    /* TODO timeout */

    while (!(bp->b_flags & B_DONE));

    return bp->b_error;
}

int biowait(struct buf * bp)
{
    return _biowait(bp, 0);
}

static void bio_idle_task(void)
{
    /* TODO look for unbusied buffers that can be written out. */
    /* TODO vrfree buffers that have been unused for a long time. */
}
DATA_SET(sched_idle_tasks, bio_idle_task);
