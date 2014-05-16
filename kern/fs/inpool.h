/**
 *******************************************************************************
 * @file    inpool.h
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

#pragma once
#ifndef INPOOL_H
#define INPOOL_H

#ifndef FS_H
#error <fs.h> must be included.
#endif

/**
 * typedef for callback to the inode creation function.
 * @param sb is the superblock used.
 * @param num is the inode number used.
 */
typedef vnode_t * (*inpool_crin_t)(const fs_superblock_t * sb, ino_t * num);

/**
 * inode pool struct.
 * The implementation of inode pool uses vnodes to make the implementation more
 * generic, this means that vnode has to be defined as a static member in the
 * actual inode struct.
 */
typedef struct inpool {
    vnode_t ** ip_arr; /*!< inode pool array. */
    size_t ip_max; /*!< Maximum size of the inode pool. */
    size_t ip_wr; /*!< Write index. */
    size_t ip_rd; /*!< Read index. */
    size_t ip_next_inum; /*!< Next free inode number after pool is used. */
    fs_superblock_t * ip_sb; /*!< Default Super block of this pool. */

    inpool_crin_t create_inode; /*!< Create inode callback. */
} inpool_t;

int inpool_init(inpool_t * pool, fs_superblock_t * sb,
        inpool_crin_t create_inode, size_t max);
vnode_t * inpool_insert(inpool_t * pool, vnode_t * vnode);
vnode_t * inpool_get_next(inpool_t * pool);
void inpool_destroy(inpool_t * pool);

#endif /* INPOOL_H */
