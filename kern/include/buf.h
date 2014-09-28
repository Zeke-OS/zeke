/**
 *******************************************************************************
 * @file    buf.h
 * @author  Olli Vanhoja
 * @brief   Buffer Cache.
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
#ifndef BUF_H
#define BUF_H

#include <llist.h>
#include <hal/mmu.h>
#include <fs/fs.h>
#include <vm/vm.h>

/**
 * @addtogroup buffercache vralloc bread breadn bwrite bawrite bdwrite getblk geteblk incore allocbuf brelse biodone biowait
 * The buffercache interface is used by filesystems to improve I/O performance
 * using in-core caches of filesystem blocks.
 * The kernel memory used to cache a block is called a buffer and described
 * by a buf structure. In addition to describing a cached block, a buf
 * structure is also used to describe an I/O request as a part of the disk
 * driver interface and for allocating and mapping memory for user space.
 * @{
 */

/**
 * VM memory region management structure.
 * This struct type is used to manage memory regions in the vm system.
 */
struct buf {
    uintptr_t b_data;       /*!< Address in kernel space. */
    size_t b_bufsize;       /*!< Allocated buffer size. */
    size_t b_bcount;        /*!< Originally requested buffer size, can be used
                             *   for bounds check. */
    size_t b_blkno;         /*!< Block # on device. */
    size_t b_lblkno;        /*!< Logical block number. */

    /* MMU mappings.             Usually used for user space mapping. */
    mmu_region_t b_mmu;     /*!< MMU struct for user space or special access. */
    int b_uflags;           /*!< Actual user space permissions and flags. */

    /* IO Buffer */
    file_t b_file;          /*!< File descriptor for the buffered vnode. */
    file_t b_devfile;       /*!< File descriptor for the buffered device. */
    size_t b_dirtyoff;      /*!< Offset in buffer of dirty region. */
    size_t b_dirtyend;      /*!< Offset of end of dirty region. */

    /* Status */
    unsigned b_flags;
    int b_error;            /*!< Negative errno returned after I/O. */
    size_t b_resid;         /*!< words not transferred after an error. */

    /** Operations */
    const struct vm_ops * vm_ops;

    void * allocator_data;  /*!< Allocator specific data. */
    SPLAY_ENTRY(buf) sentry_;
    llist_nodedsc_t lentry_;

    int refcount;
    mtx_t lock;
};

/**
 * VM operations.
 */
typedef struct vm_ops {
    /**
     * Increment region reference count.
     */
    void (*rref)(struct buf * this);

    /**
     * Pointer to a 1:1 region cloning function.
     * This function if set clones contents of the region to another
     * physical locatation.
     * @note Can be null.
     * @param cur_region    is a pointer to the current region.
     */
    struct buf * (*rclone)(struct buf * old_region);

    /**
     * Free this region.
     * @note Can be null.
     * @param this is the current region.
     */
    void (*rfree)(struct buf * this);
} vm_ops_t;

#define B_DONE      0x00002     /*!< Transaction finished. */
#define B_ERROR     0x00004     /*!< Transaction aborted. */
#define B_BUSY      0x00008     /*!< Buffer busy. */
#define B_LOCKED    0x00010     /*!< Locked in memory. */
#define B_DIRTY     0x00020
#define B_NOCOPY    0x00100     /*!< Don't copy-on-write this buf. */
#define B_ASYNC     0x01000     /*!< Start I/O but don't wait for completion. */
#define B_DELWRI    0x04000     /*!< Delayed write. */
#define B_IOERROR   0x10000     /*!< IO Error. */

#define BUF_LOCK(bp)    mtx_lock(&(bp)->lock)
#define BUF_UNLOCK(bp)  mtx_unlock(&(bp)->lock)

int biobuf_compar(struct buf * a, struct buf * b);
SPLAY_PROTOTYPE(bufhd_splay, buf, sentry_, biobuf_compar);

/**
 * Read a block corresponding to vnode and blkno.
 * If the buffer is not found (i.e. the block is not cached in memory,
 * bread() calls getblk() to allocate a buffer with enough pages for
 * size and reads the specified disk block into it. The buffer returned
 * by bread() is marked as busy. (The B_BUSY  flag is set.) After manipulation
 * of the buffer returned from bread(), the caller should unbusy it so that
 * another thread can get it. If the buffer contents are modified and should be
 * written back to disk, it should be unbusied using one of the variants of
 * bwrite().  Otherwise, it should be unbusied using brelse().
 * @param[in]   vnode   is a pointer to a vnode.
 * @param[in]   blkno   is a block number.
 * @param[in]   size    is the size to be read.
 * @param[out]  buf     points to the returned buffer.
 * @return      Returns 0 if succeed; A negative errno if failed.
 */
int bread(vnode_t * vnode, size_t blkno, int size, struct buf ** bpp);

/**
 * Get a buffer as bread().
 * In addition, breadn() will start read-ahead of blocks specified by rablks,
 * rasizes and nrablks. The read-ahead blocks aren't returned, but are available
 * in cache for future accesses.
 * @param[in]   vnode   is a pointer to a vnode.
 * @param[in]   blkno   is a block number.
 * @param[in]   size    is the size to be read.
 * @param[in]   rablks
 * @param[in]   rasizes
 * @param[in]   nrablks
 * @param[out]  bpp     points to the returned buffer.
 * @return      Returns 0 if succeed; A negative errno if failed.
 */
int  breadn(vnode_t * vnode, size_t blkno, int size, size_t rablks[],
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
 * Clear a buffer.
 */
void bio_clrbuf(struct buf * bp);

/**
 * Get a block of requested size size that is associated with a given vnode and
 * block offset, specified by vp and blkno.
 * If the block is found in the cache, mark it as having been found, make it
 * busy and return. Otherwise, return an empty block of the correct size.  It
 * is up to the caller to ensure that the cache blocks are of the correct size.
 */
struct buf * getblk(vnode_t * vnode, size_t blkno, size_t size, int slptimeo);

/**
 * Allocate an empty, disassociated block of a given size size.
 * @param[in] size is the size of the new buffer.
 * @return  Returns the new buffer.
 */
struct buf * geteblk(size_t size);

/**
 * Get a special block that has a mapping in ksect area as well as regular
 * mapping in kernel space.
 * This buffer can be used, for example, to access memory mapped hardware
 * by setting strongly orderd access to control. Regular buffers may miss
 * newly written data due to CPU caching.
 */
struct buf * geteblk_special(size_t size, uint32_t control);

/**
 * Determine if a block associated with a given vnode and block offset is in
 * the cache.
 * @param[in]   vnode   is a vnode pointer.
 * @param[in]   blkno   is the block number.
 * @return  Returns the buffer if it's already in memory.
 */
struct buf * incore(vnode_t * vnode, size_t blkno);

/**
 * Expand or contract a allocated buffer.
 * If the buffer shrinks, the truncated part of the data is lost, so it is up
 * to the caller to have written it out first if needed; this routine will not
 * start a write.  If the buffer grows, it is the caller's responsibility to
 * fill out the buffer's additional contents.
 * @param[in] buf   is the buffer.
 * @param[in] size  is the new size.
 */
void allocbuf(struct buf * bp, size_t size);

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

int bio_geterror(struct buf * bp);

/**
 * Clone a vregion.
 * @param old_region is the old region to be cloned.
 * @return  Returns pointer to the new vregion if operation was successful;
 *          Otherwise zero.
 */
struct buf * vr_rclone(struct buf * old_region);

/**
 * Free allocated vregion.
 * Dereferences a vregion.
 * @param region is a vregion to be derefenced/freed.
 */
void vrfree(struct buf * region);

/**
 * @}
 */

#endif /* BUF_H */
