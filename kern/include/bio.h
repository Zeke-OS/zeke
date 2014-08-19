/**
 *******************************************************************************
 * @file    bio.h
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

#pragma once
#ifndef BIO_H
#define BIO_H

#include <fs/fs.h>

/**
 * Read a block corresponding to vnode and blkno.
 * @param[in]   vnode   is a pointer vnode to a vnode.
 * @param[in]   blkno   is a block number.
 * @param[in]   size    is the size to be read.
 * @param[out]  buf     points to the returned buffer.
 * @return      Returns 0 if succeed; A negative errno if failed.
 */
int bread(vnode_t * vnode, off_t blkno, int size, struct buf ** bpp);

/**
 * Get a buffer as bread().
 * In addition, breadn() will start read-ahead of blocks specified by rablks,
 * rasizes and nrablks. The read-ahead blocks aren't returned, but are available
 * in cache for future accesses.
 * @param[in]   vnode   is a pointer vnode to a vnode.
 * @param[in]   blkno   is a block number.
 * @param[in]   size    is the size to be read.
 * @param[in]   rablks
 * @param[in]   rasizes
 * @param[in]   nrablks
 * @param[out]  bpp     points to the returned buffer.
 * @return      Returns 0 if succeed; A negative errno if failed.
 */
int  breadn(vnode_t * vnode, off_t blkno, int size, off_t rablks[],
            int rasizes[], int nrablks, struct buf ** bpp);

/**
 * Write a block.
 * This will block until IO is complete.
 * @param[in] buf   is the associated buffer.
 * @return  0 if IO was complete; -EIO in case of IO error.
 */
int bwrite(struct buf * bp);

/**
 * Write a block asynchronously.
 * @param[in] buf   is the associated buffer.
 */
void bawrite(struct buf * bp);

/**
 * Delayed write.
 * @param[in] buf   is the associated buffer.
 */
void bdwrite(struct buf * bp);

/**
 * Get a block of requested size size that is associated with a given vnode and
 * block offset, specified by vp and blkno.
 * If the block is found in the cache, mark it as having been found, make it
 * busy and return. Otherwise, return an empty block of the correct size.  It
 * is up to the caller to ensure that the cache blocks are of the correct size.
 */
struct buf * getblk(vnode_t * vnode, off_t blkno, int size, int slpflag,
                    int slptimeo);

/**
 * Allocate an empty, disassociated block of a given size size.
 * @param[in] size is the size of the new buffer.
 * @return  Returns the new buffer.
 */
struct buf * geteblk(int size);

/**
 * Determine if a block associated with a given vnode and block offset is in
 * the cache.
 * @param[in]   vnode   is a vnode pointer.
 * @param[in]   blkno   is the block number.
 * @return  Returns the buffer if it's already in memory.
 */
struct buf * incore(vnode_t * vnode, off_t blkno);

/**
 * Expand or contract a allocated buffer.
 * @param[in] buf   is the buffer.
 * @param[in] size  is the new size.
 */
void allocbuf(struct buf * bp, int size);

/**
 * Unlock a buffer.
 * Clears all flags and add to free list.
 * @param[in] buf   is the buffer.
 */
void brelse(struct buf * bp);

/**
 * Mark I/O complete on a buffer.
 * @param[in] buf   is the buffer.
 */
void biodone(struct buf * bp);

/**
 * Wait for operations on the buffer to complete.
 * @param[in] buf   is the buffer.
 * @return  Returns 0 if IO was complete; In case of IO error -EIO.
 */
int biowait(struct buf * bp);

#endif /* BIO_H */
