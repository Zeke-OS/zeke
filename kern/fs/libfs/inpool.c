/**
 *******************************************************************************
 * @file    inpool.c
 * @author  Olli Vanhoja
 * @brief   Generic inode pool.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <stddef.h>
#include <errno.h>
#include <kerror.h>
#include <kmalloc.h>
#include <fs/fs.h>
#include <fs/inpool.h>

static size_t inpool_fill(inpool_t * pool, size_t count);

int inpool_init(inpool_t * pool, fs_superblock_t * sb,
                inpool_creatin_t create_inode,
                inpool_destrin_t destroy_inode,
                size_t max)
{
    int retval = 0;

    mtx_init(&pool->lock, MTX_TYPE_SPIN);
    mtx_lock(&pool->lock);

    pool->ip_max = max;
    pool->ip_count = 0;
    pool->ip_next_inum = 0;
    pool->ip_sb = sb;
    pool->create_inode = create_inode;
    pool->destroy_inode = destroy_inode;
    TAILQ_INIT(&pool->ip_freelist);
    TAILQ_INIT(&pool->ip_dirtylist);

    if (inpool_fill(pool, max) == 0)
        retval = -ENOMEM;

    mtx_unlock(&pool->lock);

    return retval;
}

void inpool_destroy(inpool_t * pool)
{
    vnode_t * vnode;

    pool->ip_max = 0;

    /* Delete vnodes stored in pool. */
    while (!TAILQ_EMPTY(&pool->ip_freelist)) {
        vnode = TAILQ_FIRST(&pool->ip_freelist);
        TAILQ_REMOVE(&pool->ip_freelist, vnode, vn_inqueue);
        pool->destroy_inode(vnode);
    }

    /* Delete dirty vnodes */
    while (!TAILQ_EMPTY(&pool->ip_dirtylist)) {
        vnode = TAILQ_FIRST(&pool->ip_dirtylist);
        TAILQ_REMOVE(&pool->ip_dirtylist, vnode, vn_inqueue);
        pool->destroy_inode(vnode);
    }
}

void inpool_insert(inpool_t * pool, vnode_t * vnode)
{
    mtx_lock(&pool->lock);
    TAILQ_INSERT_TAIL(&pool->ip_dirtylist, vnode, vn_inqueue);
    mtx_unlock(&pool->lock);
}

vnode_t * inpool_get_next(inpool_t * pool)
{
    vnode_t * vnode;

    mtx_lock(&pool->lock);

    if (TAILQ_EMPTY(&pool->ip_freelist)) {
        int n = inpool_fill(pool, pool->ip_max / 2);
        if (n < 1)
            return NULL;
    }

    vnode = TAILQ_FIRST(&pool->ip_freelist);
    TAILQ_REMOVE(&pool->ip_freelist, vnode, vn_inqueue);
    pool->ip_count--;

    mtx_unlock(&pool->lock);

    return vnode;
}

/**
 * Fill the inode pool.
 * @param pool  is the pool to be filled.
 * @param count is the number of inodes to be filled in.
 * @return Returns the number of inodes inserted into the pool.
 */
static size_t inpool_fill(inpool_t * pool, size_t count)
{
    int i = 0;
    vnode_t * vnode;
    vnode_t * vnode_temp;

    KASSERT(mtx_test(&pool->lock), "pool should be locked.");

    /*
     * First fill from the "dirty" list.
     * Purpose of the dirty list is to try avoid remapping or destroying a vnode
     * that may still undergo some access by some process. TODO We still should
     * try to come up with a better solution here for concurrency safety.
     */
    TAILQ_FOREACH_SAFE(vnode, &pool->ip_dirtylist, vn_inqueue, vnode_temp) {
        if (vrefcnt(vnode) > 1)
            continue;
        TAILQ_REMOVE(&pool->ip_dirtylist, vnode, vn_inqueue);

        if (count > 0 && pool->ip_count < pool->ip_max) {
            /* Insert into the free list. */
            TAILQ_INSERT_TAIL(&pool->ip_freelist, vnode, vn_inqueue);
            pool->ip_count++;
            pool->ip_next_inum++;
            i++;
            count--;
        } else {
            /*
             * Destroy it as it's not needed anymore and can't fit into
             * the free list.
             */
            pool->destroy_inode(vnode);
        }
    }

    /* Insert some new inodes if necessary. */
    while (count-- > 0 && pool->ip_count < pool->ip_max) {
        ino_t * num = &(pool->ip_next_inum);

        vnode = pool->create_inode(pool->ip_sb, num);
        if (!vnode)
            break;

        TAILQ_INSERT_TAIL(&pool->ip_freelist, vnode, vn_inqueue);
        pool->ip_count++;
        pool->ip_next_inum++;
        i++;
    }

    return i;
}
