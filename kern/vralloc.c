/**
 *******************************************************************************
 * @file    vralloc.c
 * @author  Olli Vanhoja
 * @brief   Virtual Region Allocator.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <sys/sysctl.h>
#include <bitmap.h>
#include <buf.h>
#include <dllist.h>
#include <dynmem.h>
#include <errno.h>
#include <kerror.h>
#include <kinit.h>
#include <kmalloc.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <ptmapper.h>
#include <vm/vm.h>

/**
 * vralloc region struct.
 * Struct describing a single dynmem alloc block of vrallocated memory.
 * This is the internal representation of a memory allocation made with
 * vralloc where as struct buf is the external interface used and seen
 * by external users.
 */
struct vregion {
    llist_nodedsc_t node;
    uintptr_t kaddr;    /*!< Kernel address of the allocated dynmem block. */
    int count;          /*!< Reserved pages count. */
    size_t size;        /*!< Size of allocation bitmap in bytes. */
    bitmap_t map[0];    /*!< Bitmap of reserved pages. */
};

#define VREG_SIZE(count) \
    (sizeof(struct vregion) + E2BITMAP_SIZE(count) * sizeof(bitmap_t))

#define VREG_PCOUNT(byte_size_) \
    ((byte_size_) / MMU_PGSIZE_COARSE)

#define VREG_BYTESIZE(pcount_) \
    ((pcount_) * MMU_PGSIZE_COARSE)

#define VREG_I2ADDR(vreg_, iblock_) \
    ((vreg_)->kaddr + VREG_BYTESIZE(iblock_))

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

static struct vregion * vreg_alloc_node(size_t count);
static int get_iblocks(size_t * iblock, size_t pcount,
                      struct vregion ** vreg_ret);
static void vrref(struct buf * region);

static llist_t * vrlist; /*!< List of all allocations done by vralloc. */
static struct vregion * last_vreg; /*!< Last node that contained empty pages. */
static mtx_t vr_big_lock;

static size_t vmem_all;
SYSCTL_UINT(_vm, OID_AUTO, vmem_all, CTLFLAG_RD, &vmem_all, 0,
    "Amount of memory currently allocated");

static size_t vmem_used;
SYSCTL_UINT(_vm, OID_AUTO, vmem_used, CTLFLAG_RD, &vmem_used, 0,
    "Amount of memory used");



/**
 * VRA specific operations for allocated vm regions.
 */
static const vm_ops_t vra_ops = {
    .rref = vrref,
    .rclone = vr_rclone,
    .rfree = vrfree
};


/**
 * Initializes vregion allocator data structures.
 */
int __kinit__ vralloc_init(void)
{
    extern void _bio_init(void);
    SUBSYS_INIT("vrallloc");
    struct vregion * reg;

    mtx_init(&vr_big_lock, MTX_TYPE_TICKET, 0);

    vrlist = dllist_create(struct vregion, node);

    mtx_lock(&vr_big_lock);
    reg = vreg_alloc_node(256);
    if (!(vrlist && reg)) {
        /* No need to unlock big lock */
        return -ENOMEM;
    }
    mtx_unlock(&vr_big_lock);

    last_vreg = reg;

    _bio_init();

    return 0;
}

/**
 * Allocate a new vregion node/chunk and memory for the region.
 * @param count is the page count (4kB pages). Should be multiple of 256;
 *        Otherwise it will be rounded up.
 * @return Rerturns a pointer to the newly allocated region; Otherwise 0.
 */
static struct vregion * vreg_alloc_node(size_t count)
{
    struct vregion * vreg;

    KASSERT(mtx_test(&vr_big_lock), "vr_big_lock should be locked\n");

    count = ROUND_UP(count, 256);

    vreg = kzalloc(VREG_SIZE(count));
    if (!vreg)
        return NULL;

    vreg->kaddr = (uintptr_t)dynmem_alloc_region(count / 256, MMU_AP_RWNA,
                                                 MMU_CTRL_MEMTYPE_WB);
    if (vreg->kaddr == 0) {
        kfree(vreg);
        return NULL;
    }

    vreg->size = E2BITMAP_SIZE(count) * sizeof(bitmap_t);

    vrlist->insert_head(vrlist, vreg);
    /* Update stats */
    vmem_all += VREG_BYTESIZE(count);

    return vreg;
}

/**
 * Get pcount number of unallocated pages.
 * @note needs to get vr_big_lock.
 * @param[out] iblock is the returned index of the allocation made.
 * @param pcount is the number of pages requested.
 * @param vreg_ret[out] returns a pointer to the allocated vreg.
 */
static int get_iblocks(size_t * iblock, size_t pcount,
                       struct vregion ** vreg_ret)
{
    struct vregion * vreg = last_vreg;
    int retval;

    mtx_lock(&vr_big_lock);

retry_vra:
    do {
        if (bitmap_block_search(iblock, pcount, vreg->map, vreg->size) == 0)
            break; /* Found block */
    } while ((vreg = vreg->node.next));

    if (!vreg) { /* Not found */
        vreg = vreg_alloc_node(pcount);
        if (!vreg) {
            retval = -ENOMEM;
            goto out;
        }
        goto retry_vra;
    }

    bitmap_block_update(vreg->map, 1, *iblock, pcount);
    *vreg_ret = vreg;

    retval = 0;
out:
    mtx_unlock(&vr_big_lock);
    return retval;
}

/**
 * vregion free callback.
 * This function is called by kobj.
 */
static void vreg_free_callback(struct kobj * obj)
{
    struct buf * bp = container_of(obj, struct buf, b_obj);
    const size_t bcount = VREG_PCOUNT(bp->b_bufsize);
    struct vregion * vreg;
    size_t iblock;

    /*
     * Get vreg pointer and iblock no.
     */
    vreg = (struct vregion *)(bp->allocator_data);
    iblock = VREG_PCOUNT(bp->b_data - vreg->kaddr);

    bitmap_block_update(vreg->map, 0, iblock, bcount);
    vreg->count -= bcount;

    mtx_lock(&vr_big_lock);
    vmem_used -= bp->b_bufsize; /* Update stats */
    mtx_unlock(&vr_big_lock);

    kfree(bp);
    /* Don't use bp beoynd this point */

    mtx_lock(&vr_big_lock);
    if (vreg->count <= 0 && vreg == last_vreg) {
        /**
         * Free the vregion node.
         */
        /* Update stats */
        vmem_all -= vreg->size * (4 * 8) * MMU_PGSIZE_COARSE;

        /* Free node */
        vrlist->remove(vrlist, vreg);
        mtx_unlock(&vr_big_lock);

        dynmem_free_region((void *)vreg->kaddr);
        kfree(vreg);
    } else {
        mtx_unlock(&vr_big_lock);
    }
}

struct buf * geteblk(size_t size)
{
    size_t iblock; /* Block index of the allocation */
    const size_t orig_size = size;
    size = memalign_size(size, MMU_PGSIZE_COARSE);
    const size_t pcount = VREG_PCOUNT(size);
    struct vregion * vreg;
    struct buf * bp = NULL;

    bp = kzalloc(sizeof(struct buf));
    if (!bp) {
#if defined(configBUF_DEBUG)
        KERROR(KERROR_DEBUG, "%s: Can't allocate vm_region struct\n", __func__);
#endif
        return NULL;
    }

    if (get_iblocks(&iblock, pcount, &vreg)) {
#if defined(configBUF_DEBUG)
        KERROR(KERROR_DEBUG, "%s: Can't get vregion for a new buffer\n",
               __func__);
#endif
        kfree(bp);
        return NULL;
    }

    mtx_init(&bp->lock, MTX_TYPE_TICKET, 0);

    /* Update target struct */
    bp->b_mmu.paddr = VREG_I2ADDR(vreg, iblock);
    bp->b_mmu.num_pages = pcount;
    bp->b_data = bp->b_mmu.paddr; /* Currently this way as
                                   * kernel space is 1:1 */
    bp->b_bufsize = VREG_BYTESIZE(pcount);
    bp->b_bcount = orig_size;
    bp->b_flags = B_BUSY;
    kobj_init(&bp->b_obj, vreg_free_callback);
    bp->allocator_data = vreg;
    bp->vm_ops = &vra_ops;
    bp->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
    vm_updateusr_ap(bp);

    vreg->count += pcount;

    /* Update stats */
    mtx_lock(&vr_big_lock);
    vmem_used += size;
    last_vreg = vreg;
    mtx_unlock(&vr_big_lock);

    /* Clear allocated pages. */
    memset((void *)bp->b_data, 0, bp->b_bufsize);

    return bp;
}

/**
 * Increment reference count of a vr allocated vm_region.
 * @param region is a vregion.
 */
static void vrref(struct buf * bp)
{
    kobj_ref(&bp->b_obj);
}

struct buf * vr_rclone(struct buf * old_region)
{
    struct buf * new_region;
    const size_t rsize = old_region->b_bufsize;

    /* "Lock", ensure that the region is not freed during the operation. */
    vrref(old_region);

    new_region = geteblk(rsize);
    if (!new_region) {
        KERROR(KERROR_ERR, "%s: Out of memory, tried to allocate %d bytes\n",
                __func__, rsize);
        return NULL;
    }

#if defined(configBUF_DEBUG)
    KERROR(KERROR_DEBUG, "clone %x -> %x, %u bytes\n",
           old_region->b_data, new_region->b_data, rsize);
#endif

    /* Copy data */
    memcpy((void *)(new_region->b_data), (void *)(old_region->b_data),
            rsize);

    /* Copy attributes */
    new_region->b_uflags = ~VM_PROT_COW & old_region->b_uflags;
    new_region->b_mmu.vaddr = old_region->b_mmu.vaddr;
    /* num_pages already set */
    new_region->b_mmu.ap = old_region->b_mmu.ap;
    new_region->b_mmu.control = old_region->b_mmu.control;
    /* paddr already set */
    new_region->b_mmu.pt = old_region->b_mmu.pt;
    vm_updateusr_ap(new_region);

    /* Release "lock". */
    vrfree(old_region);

    return new_region;
}

void allocbuf(struct buf * bp, size_t size)
{
    const size_t orig_size = size;
    const size_t new_size = memalign_size(size, MMU_PGSIZE_COARSE);
    const size_t pcount = VREG_PCOUNT(new_size);
    const size_t blockdiff = pcount - VREG_PCOUNT(bp->b_bufsize);
    size_t iblock;
    size_t bcount = VREG_PCOUNT(bp->b_bufsize);
    struct vregion * vreg = bp->allocator_data;

    KASSERT(vreg, "bp->allocator_data should be always set");

    if (bp->b_bufsize == new_size)
        return;

    mtx_lock(&bp->lock);

    if (blockdiff > 0) {
        const size_t sblock = VREG_PCOUNT((bp->b_data - vreg->kaddr)) + bcount;

        if (bitmap_block_search_s(sblock, &iblock, blockdiff, vreg->map,
                    vreg->size) == 0) {
            if (iblock == sblock + 1) {
                bitmap_block_update(vreg->map, 1, sblock, blockdiff);
            } else { /* Must allocate a new region */
                struct vregion * nvreg;
                uintptr_t new_addr;

                if (get_iblocks(&iblock, pcount, &nvreg)) {
                    /*
                     * It's not nice to panic here but we don't have any
                     * method to inform the caller about OOM.
                     */
                    panic("OOM during allocbuf()");
                }

                new_addr = VREG_I2ADDR(nvreg, iblock);
                memcpy((void *)(new_addr), (void *)(bp->b_data), bp->b_bufsize);

                bp->b_mmu.paddr = new_addr;
                bp->b_data = bp->b_mmu.paddr; /* Currently this way as
                                               * kernel space is 1:1 */
                bp->allocator_data = nvreg;

                /* Free blocks from old vreg */
                bitmap_block_update(vreg->map, 0, iblock, bcount);
                vreg = nvreg;

                mtx_unlock(&bp->lock);
            }
        }
    } else {
#if 0 /*
       * We don't usually want to shrunk because it's hard to get memory back.
       */
        const size_t sblock = VREG_PCOUNT((bp->b_data - vreg->kaddr)) + bcount +
                              blockdiff;

        bitmap_block_update(vreg->map, 0, sblock, -blockdiff);
#endif
    }

    vreg->count += blockdiff;
    bp->b_bufsize = new_size;
    bp->b_bcount = orig_size;
    bp->b_mmu.num_pages = pcount;

    mtx_unlock(&bp->lock);
}

void vrfree(struct buf * bp)
{
    KASSERT(bp != NULL, "bp can't be NULL");

    kobj_unref(&bp->b_obj);
}

int clone2vr(struct buf * src, struct buf ** out)
{
    struct buf * new;

    KASSERT(src != NULL, "src arg must be set");
    KASSERT(out != NULL, "out arg must be set");

    if (src->vm_ops == &vra_ops && src->vm_ops->rclone) {
        /* If the buffer is vrallocated already we can just call rclone(). */
        new = src->vm_ops->rclone(src);
        if (!new) {
            return -ENOMEM;
        }
    } else if (src->b_data != 0) {
        /*
         * Not vregion, try to clone manually.
         * b_data is expected to be zero if the data is not in memory.
         */
        const size_t rsize = src->b_bufsize;

        new = geteblk(rsize);
        if (!new) {
            return -ENOMEM;
        }

        /* RFE new->b_data instead? */
        memcpy((void *)(new->b_mmu.paddr), (void *)(src->b_data), rsize);
        new->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
        new->b_mmu.vaddr = src->b_mmu.vaddr;
        new->b_mmu.ap = src->b_mmu.ap;
        new->b_mmu.control = src->b_mmu.control;
        /* paddr already set */
        new->b_mmu.pt = src->b_mmu.pt;
        vm_updateusr_ap(new);
    } else {
        /* TODO else if data not in mem */
        return -ENOTSUP;
    }

    *out = new;
    return 0;
}
