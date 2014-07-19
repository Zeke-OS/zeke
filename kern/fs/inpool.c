/**
 *******************************************************************************
 * @file    inpool.c
 * @author  Olli Vanhoja
 * @brief   Generic inode pool.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kmalloc.h>
#include <fs/fs.h>
#include "inpool.h"

static size_t inpool_fill(inpool_t * pool, size_t count);

/**
 * Initialize a inode pool.
 * Allocates memory for a inode pool according to parameter max.
 * @param pool  is the inode pool struct that is initialized.
 * @param sb    is the default super block of this pool.
 * @param max   is maximum size of initialized inode pool.
 * @return Return value is 0 if succeeded; Otherwise value other than zero.
 */
int inpool_init(inpool_t * pool, fs_superblock_t * sb,
        inpool_crin_t create_inode, size_t max)
{
    int retval = 0;

    /* Create the pool array. */
    pool->ip_arr = kcalloc(max, sizeof(void *));
    if (pool->ip_arr == 0) { /* OOM */
        retval = -1;
        goto out;
    }

    pool->ip_max = max + 1;
    pool->ip_wr = 0;
    pool->ip_rd = 0;
    pool->ip_next_inum = 0;
    pool->ip_sb = sb;
    pool->create_inode = create_inode;

    /* Pool is not fully filled so that wr & rd pointer won't point to the same
     * index. */
    inpool_fill(pool, max);

out:
    return retval;
}

/**
 * Destroy a inode pool.
 * @param pool is the inode pool to be destroyed.
 */
void inpool_destroy(inpool_t * pool)
{
    size_t i;
    vnode_t * vnode;

    pool->ip_max = 0;

    /* Delete each vnode. */
    for (i = 0; i < pool->ip_max; i++) {
        if (pool->ip_arr[i] != 0) {
            vnode = pool->ip_arr[i];
            if (vnode->sb != 0) /* If sb is null we'll leak some meory. */
                vnode->sb->delete_vnode(vnode);
            pool->ip_arr[i] = 0;
        }
    }

    kfree(pool->ip_arr);
    pool->ip_arr = 0;
}

/**
 * Insert inode to the inode pool.
 * This function can be used for inode recycling.
 * @note If pool is full inode is destroyed and the inode number is lost from
 *       recycling until the pool is reinitialized.
 * @param pool  is the inode pool, vnode_num must be set and refcount should have
 *              sane value.
 * @param vnode is the inode that will be inserted to the pool.
 * @return  Returns null if vnode was inserted to the pool; Otherwise returns a
 *          vnode that could not be fitted to the pool.
 */
vnode_t * inpool_insert(inpool_t * pool, vnode_t * vnode)
{
    size_t next;

    if (pool->ip_max == 0)
        goto out;

    next = (pool->ip_wr + 1) % pool->ip_max;

    if (next == pool->ip_rd) {
        /* Pool is closed... ehm full, can't fit any more inodes to the pool.
         * So we must return back this node.
         */
        goto out;
    } else {
        /* Insert. */
        pool->ip_arr[pool->ip_wr] = vnode;
        pool->ip_wr = next;
        vnode = 0; /* Set return value to null. */
    }

out:
    return vnode;
}

/**
 * Get the next free node from the inode pool.
 * @param pool is the pool where inode is removed from.
 * @return  Returns a new inode from the pool or null pointer if out of memory
 *          or out of inode numbers.
 */
vnode_t * inpool_get_next(inpool_t * pool)
{
    size_t rd = pool->ip_rd;
    size_t wr = pool->ip_wr;
    size_t next;
    vnode_t * retval = 0;

    if (pool->ip_max == 0)
        goto out;

    if (rd == wr) { /* Pool is empty. */
        size_t n;

        /* Fill the pool. */
        n = inpool_fill(pool, pool->ip_max / 2);

        if (n < 1) /* Could not allocate even at least one inode. */
            goto out;
    }

    next = (rd  + 1) % pool->ip_max;
    retval = pool->ip_arr[rd];
    pool->ip_rd = next;

out:
    return retval;
}

/**
 * Fill the inode pool.
 * @param pool  is the pool to be filled.
 * @param count is the number of inodes to be filled in.
 * @return Returns the number of inodes inserted to the pool.
 */
static size_t inpool_fill(inpool_t * pool, size_t count)
{
    size_t i = 0;
    vnode_t * vnode;

    for (; i < count; i++) {
        ino_t * num = &(pool->ip_next_inum);
        vnode = pool->create_inode(pool->ip_sb, num);
        if (vnode == 0)
            break;
        inpool_insert(pool, vnode);
        pool->ip_next_inum++;
    }

    return i;
}
