/**
 *******************************************************************************
 * @file    inpool.h
 * @author  Olli Vanhoja
 * @brief   Generic inode pool.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef INPOOL_H
#define INPOOL_H

#ifndef FS_H
#error <fs.h> must be included.
#endif

#include <sys/queue.h>

/**
 * typedef for callback to the inode creation function.
 * @param sb is the superblock used.
 * @param num is the inode number used.
 * TODO inpool shouldn't touch ino_t
 */
typedef vnode_t * inpool_creatin_t(const struct fs_superblock * sb,
                                   ino_t * num);
typedef void      inpool_destrin_t(vnode_t * vnode);
/**
 * Sync inode and destroy all cached data.
 */
typedef void      inpool_finalizein_t(vnode_t * vnode);

TAILQ_HEAD(ip_listhead, vnode);

/**
 * inode pool struct.
 * The implementation of inode pool uses vnodes to make the implementation more
 * generic, this means that vnode has to be defined as a static member in the
 * actual inode struct.
 */
typedef struct inpool {
    struct ip_listhead ip_freelist;
    struct ip_listhead ip_dirtylist;
    size_t ip_count;
    size_t ip_max;              /*!< Maximum size of the inode pool. */
    ino_t ip_next_inum;         /*!< Next free in number after pool is empty. */
    struct fs_superblock * ip_sb; /*!< Default Super block of this pool. */
    mtx_t lock;

    inpool_creatin_t * create_inode;        /*!< Create inode callback. */
    inpool_destrin_t * destroy_inode;       /*!< Destroy inode callback. */
    /**
     * Sync and destroy all cached data linked to the inode, thus finalize.
     * This callback is optional and can be set NULL.
     */
    inpool_finalizein_t * finalize_inode;
} inpool_t;



/**
 * Initialize a inode pool.
 * Allocates memory for a inode pool according to parameter max.
 * @param pool  is the inode pool struct that is initialized.
 * @param sb    is the default super block of this pool.
 * @param max   is maximum size of initialized inode pool.
 * @return Return value is 0 if succeeded; Otherwise value other than zero.
 */
int inpool_init(inpool_t * pool, struct fs_superblock * sb,
                inpool_creatin_t create_inode,
                inpool_destrin_t destroy_inode,
                inpool_finalizein_t * finalize_inode,
                size_t max);

/**
 * Insert a clean inode to the inode pool.
 * This function can be used for inode recycling.
 * @param pool  is the inode pool, vnode_num must be set and refcount should have
 *              sane value.
 * @param vnode is the inode that will be inserted to the pool.
 */
void inpool_insert_clean(inpool_t * pool, vnode_t * vnode);

/**
 * Insert a dirty inode to the inode pool.
 * @param pool  is the inode pool, vnode_num must be set and refcount should have
 *              sane value.
 * @param vnode is the inode that will be inserted to the pool.
 */
void inpool_insert_dirty(inpool_t * pool, vnode_t * vnode);

/**
 * Get the next free node from the inode pool.
 * @param pool is the pool where inode is removed from.
 * @return  Returns a new inode from the pool or null pointer if out of memory
 *          or out of inode numbers.
 */
vnode_t * inpool_get_next(inpool_t * pool);

/**
 * Destroy a inode pool.
 * @param pool is the inode pool to be destroyed.
 */
void inpool_destroy(inpool_t * pool);

#endif /* INPOOL_H */
