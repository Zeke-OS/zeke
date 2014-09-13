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
#include <sys/tree.h>
#include <errno.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kstring.h>
#include <dllist.h>
#include <fs/devfs.h>
#include <buf.h>

static mtx_t cache_lock;            /* Used to protect access caching data
                                     * structures and synchronizing access
                                     * to some functions. */
static struct llist * relse_list;  /* Released buffers list. */

static void bio_readin(struct buf * bp);
static void bio_writeout(struct buf * bp);
static void bl_brelse(struct buf * bp);
static int biowait_timo(struct buf * bp, long timeout);
static void bio_clean(int freebufs);

SPLAY_GENERATE(bufhd_splay, buf, sentry_, biobuf_compar);

/* Init bio, called by vralloc_init() */
void _bio_init(void)
{
    /*
     * TODO We'd like to use MTX_TYPE_TICKET here but bio_clean() makes it
     * impossible right now.
     */
    mtx_init(&cache_lock, MTX_TYPE_SPIN | MTX_TYPE_SLEEP |
                          MTX_TYPE_PRICEIL);
    cache_lock.pri.p_lock = NICE_MAX;

    /*
     * Init released buffers list.
     */

    relse_list = dllist_create(struct buf, lentry_);
}

/*
 * Comparator for buffer splay trees.
 */
int biobuf_compar(struct buf * a, struct buf * b)
{
#if configDEBUG >= KERROR_DEBUG
    if (a->b_file.vnode != b->b_file.vnode)
        panic("vnodes differ in the same tree");
#endif

    return (a->b_blkno - b->b_blkno);
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
    /* TODO */
    return -ENOTSUP;
}

static void bio_readin(struct buf * bp)
{
    file_t * file = &bp->b_file;

    /*
     * If we have a separate device file associated with the buffer we should
     * use it.
     */
    if (bp->b_devfile.vnode)
        file = &bp->b_devfile;

    file->seek_pos = bp->b_blkno;
    file->vnode->vnode_ops->read(file, (void *)bp->b_data, bp->b_bcount);

    bp->b_flags |= B_DONE;
}

/*
 * It's a good idea to have lock on bp before calling this function.
 */
static void bio_writeout(struct buf * bp)
{
    file_t * file = &bp->b_file;

    /*
     * If we have a separate device file associated with the buffer we should
     * use it.
     */
    if (bp->b_devfile.vnode)
        file = &bp->b_devfile;

    file->seek_pos = bp->b_blkno;
    file->vnode->vnode_ops->write(file, (void *)bp->b_data, bp->b_bcount);

    bp->b_flags |= B_DONE;
}

int bwrite(struct buf * bp)
{
    unsigned flags;
    vnode_t * vnode;
    mtx_t * lock;

#if configDEBUG >= KERROR_DEBUG
    if (!bp)
        panic("bp not set");
#endif

    lock = &bp->lock;
    vnode = bp->b_file.vnode;

    /* Sanity check */
    if(!(vnode && vnode->vnode_ops && vnode->vnode_ops->write)) {
        mtx_lock(lock);
        bp->b_flags |= B_ERROR;
        bp->b_error |= -EIO;
        mtx_unlock(lock);

        return -EIO;
    }

    mtx_lock(lock);
    flags = bp->b_flags;
    bp->b_flags &= ~(B_DONE | B_ERROR | B_ASYNC | B_DELWRI);
    bp->b_flags |= B_BUSY;
    mtx_unlock(lock);

    /* TODO Use dirty offsets */
    if (flags & B_ASYNC) {
        /* TODO */

        return 0;
    } else {
        mtx_lock(lock);
        bio_writeout(bp);
        bp->b_flags &= ~B_BUSY;
        mtx_unlock(lock);
    }

    return 0;
}

void bawrite(struct buf * bp)
{
    mtx_lock(&bp->lock);
    bp->b_flags |= B_ASYNC;
    mtx_unlock(&bp->lock);
    bwrite(bp);
}

void bdwrite(struct buf * bp)
{
    mtx_lock(&bp->lock);
    bp->b_flags |= B_DELWRI;
    mtx_unlock(&bp->lock);
}

void bio_clrbuf(struct buf * bp)
{
    unsigned flags;

#if configDEBUG >= KERROR_DEBUG
    if (!bp)
        panic("bp not set");
#endif

    mtx_lock(&bp->lock);

    flags = bp->b_flags;

    if (flags & B_DELWRI) {
        bio_writeout(bp);
    } else if (flags & B_ASYNC) {
        biowait(bp);
    }
    bp->b_flags &= ~(B_DELWRI | B_ERROR);
    bp->b_flags |= B_BUSY;
    mtx_unlock(&bp->lock);

    memset((void *)bp->b_data, 0, bp->b_bufsize);

    mtx_lock(&bp->lock);
    bp->b_flags &= ~B_BUSY;
    mtx_unlock(&bp->lock);
}

static struct buf * create_blk(vnode_t * vnode, size_t blkno, size_t size,
                               int slptimeo)
{
    struct buf * bp = geteblk(size);
    struct file file;
    struct file devfile;

    if (!bp)
        return NULL;

    bp->b_blkno = blkno;

    /* fd for the file */
    file.vnode = vnode;
    file.oflags = O_RDWR;
    file.refcount = 1;
    file.stream = NULL;

    /* fd for the device */
    if (!S_ISBLK(vnode->vn_mode) && !S_ISCHR(vnode->vn_mode)) {
        if (file.vnode->sb && file.vnode->sb->sb_dev)
            devfile.vnode = file.vnode->sb->sb_dev;
        else
            panic("file->vnode->sb->sb_dev not set");
    } else {
        panic("vn file type not supported"); /* TODO */
    }
    devfile.oflags = O_RDWR;
    devfile.refcount = 1;
    devfile.stream = NULL;

    bp->b_file = file;
    bp->b_devfile = devfile;
    mtx_init(&bp->b_file.lock, MTX_TYPE_SPIN);

    VN_LOCK(vnode);

    /* Put to the buffer splay tree of the vnode. */
    if (SPLAY_INSERT(bufhd_splay, &vnode->vn_bpo.sroot, bp))
        panic("Double insert"); /* TODO */

    VN_UNLOCK(vnode);

    return bp;
}

struct buf * getblk(vnode_t * vnode, size_t blkno, size_t size, int slptimeo)
{
    struct buf * bp;

    if (!vnode)
        return NULL;

    /* For now we want to synchronize access to this function. */
    mtx_lock(&cache_lock);

    bp = incore(vnode, blkno);
    if (!bp) { /* Not found, create a new buffer. */
        bp = create_blk(vnode, blkno, size, slptimeo);
        if (!bp)
            goto fail;
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
    mtx_lock(&bp->lock);
    if (bp->b_flags & B_BUSY) {
        mtx_unlock(&bp->lock);
        goto retry;
    }
    bp->b_flags |= B_BUSY;
    relse_list->remove(relse_list, bp); /* Remove from the released list. */
    mtx_unlock(&bp->lock);

    allocbuf(bp, size); /* Resize if necessary */

    mtx_lock(&bp->lock);
    bp->b_flags &= ~B_ERROR;
    bp->b_error = 0;
    mtx_unlock(&bp->lock);

fail:
    mtx_unlock(&cache_lock);

    return bp;
}

struct buf * incore(vnode_t * vnode, size_t blkno)
{
    struct bufhd * bf;
    struct buf * bp;
    struct buf find;

    if (!vnode)
        return NULL;

    bf = &vnode->vn_bpo;

    if (SPLAY_EMPTY(&bf->sroot))
        return NULL;

    find.b_file.vnode = vnode;
    find.b_blkno = blkno;
    bp = SPLAY_FIND(bufhd_splay, &bf->sroot, &find);

    return bp;
}

static void bl_brelse(struct buf * bp)
{
    if (mtx_test(&bp->lock))
        panic("Lock is required.");

    bp->b_flags &= ~B_BUSY;

    mtx_lock(&cache_lock);
    relse_list->insert_tail(relse_list, bp);
    mtx_unlock(&cache_lock);
}

void brelse(struct buf * bp)
{
    mtx_lock(&bp->lock);
    bl_brelse(bp);
    mtx_unlock(&bp->lock);
}

void biodone(struct buf * bp)
{
    mtx_lock(&bp->lock);

    bp->b_flags |= B_DONE;

    if (bp->b_flags & B_ASYNC)
        bl_brelse(bp);

    mtx_unlock(&bp->lock);
}

static int biowait_timo(struct buf * bp, long timeout)
{
    /* TODO timeout */

    while (!(bp->b_flags & B_DONE));

    return bp->b_error;
}

int biowait(struct buf * bp)
{
    return biowait_timo(bp, 0);
}

/**
 * Cleanup released buffers.
 * @param freebufs  tells if released buffers should be freed after write out.
 */
static void bio_clean(int freebufs)
{
    struct buf * bp;

    if (mtx_trylock(&cache_lock))
        return; /* Don't enter if we don't get exclusive access. */

    bp = relse_list->head;
    if (!bp)
        goto out; /* no nodes. */

    do {
        file_t * file;

        /* Skip if already locked or BUSY */
        if (mtx_trylock(&bp->lock) || (bp->b_flags & B_BUSY))
            continue;

        file = &bp->b_file;

        /* Write out if delayed write was set. */
        if (bp->b_flags & B_DELWRI) {
            bp->b_flags |= B_BUSY;
            bp->b_flags &= ~B_ASYNC;

            bio_writeout(bp);
        }

        if (freebufs && !(bp->b_flags & B_LOCKED) &&
                !VN_TRYLOCK(file->vnode)) {
            struct buf * bp_prev = bp->lentry_.prev;

            SPLAY_REMOVE(bufhd_splay, &file->vnode->vn_bpo.sroot, bp);
            vrfree(bp);
            VN_UNLOCK(file->vnode);

            bp = bp_prev;
            continue;
        }

        bp->b_flags &= ~B_BUSY;
        mtx_unlock(&bp->lock);
    } while ((bp = bp->lentry_.next));

out:
    mtx_unlock(&cache_lock);
}

static void bio_idle_task(void)
{
    bio_clean(0);
}
DATA_SET(sched_idle_tasks, bio_idle_task);
