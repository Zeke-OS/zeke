/**
 *******************************************************************************
 * @file    vralloc.c
 * @author  Olli Vanhoja
 * @brief   Virtual Region Allocator.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <bitmap.h>
#include <buf.h>
#include <dynmem.h>
#include <errno.h>
#include <kerror.h>
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
    LIST_ENTRY(vregion) _entry;
    uintptr_t kaddr;    /*!< Kernel address of the allocated dynmem block. */
    unsigned count;     /*!< Reserved pages count. */
#ifdef configVRALLOC_DEBUG
#define VREG_MAGIC_VALUE 0x6C542D55
    unsigned magic;
#endif
    size_t size;        /*!< Size of allocation bitmap in bytes. */
    bitmap_t map[0];    /*!< Bitmap of reserved pages. */
};

#define DMEM_BLOCK_SIZE (DYNMEM_PAGE_SIZE / MMU_PGSIZE_COARSE)

#define VREG_SIZE(count) \
    (sizeof(struct vregion) + E2BITMAP_SIZE(count) * sizeof(bitmap_t))

#define VREG_PCOUNT(byte_size_) \
    ((byte_size_) / MMU_PGSIZE_COARSE)

#define VREG_BYTESIZE(pcount_) \
    ((pcount_) * MMU_PGSIZE_COARSE)

#define VREG_I2ADDR(vreg_, iblock_) \
    ((vreg_)->kaddr + VREG_BYTESIZE(iblock_))

#define VREG_ADDR2I(vreg_, addr_) \
    (VREG_PCOUNT((addr_) - (vreg_)->kaddr))

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

static struct vregion * vreg_alloc_node(size_t count);
static void vrref(struct buf * region);
static struct buf * vr_rclone(struct buf * old_region);

/** List of all allocations done by vralloc. */
static LIST_HEAD(vrlisthead, vregion) vrlist_head =
    LIST_HEAD_INITIALIZER(vrlisthead);
static mtx_t vr_big_lock = MTX_INITIALIZER(MTX_TYPE_TICKET, MTX_OPT_DINT);

SYSCTL_DECL(_vm_vralloc);
SYSCTL_NODE(_vm, OID_AUTO, vralloc, CTLFLAG_RW, 0,
            "vralloc stats");

static size_t vralloc_all;
SYSCTL_UINT(_vm_vralloc, OID_AUTO, reserved, CTLFLAG_RD, &vralloc_all, 0,
            "Amount of memory currently allocated for vralloc");

static size_t vralloc_used;
SYSCTL_UINT(_vm_vralloc, OID_AUTO, used, CTLFLAG_RD, &vralloc_used, 0,
            "Amount of vralloc memory used");

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
 * Called from kinit.c
 */
void vralloc_init(void)
{
    extern void _bio_init(void);
    struct vregion * vreg;

    mtx_lock(&vr_big_lock);
    vreg = vreg_alloc_node(DMEM_BLOCK_SIZE);
    if (!vreg) {
        panic("vralloc initialization failed");
    }
    mtx_unlock(&vr_big_lock);

    _bio_init();
}

/**
 * Allocate a new vregion node/chunk and memory for the region.
 * @param count is the page count (4kB pages). Should be a multiple of
 *              DMEM_BLOCK_SIZE; Otherwise it will be rounded up.
 * @return Rerturns a pointer to the newly allocated region; Otherwise NULL.
 */
static struct vregion * vreg_alloc_node(size_t count)
{
    struct vregion * vreg;

    KASSERT(mtx_test(&vr_big_lock), "vr_big_lock should be locked");

    count = ROUND_UP(count, DMEM_BLOCK_SIZE);

    vreg = kzalloc(VREG_SIZE(count));
    if (!vreg)
        return NULL;

    vreg->kaddr = (uintptr_t)dynmem_alloc_region(count / DMEM_BLOCK_SIZE,
                                                 MMU_AP_RWNA,
                                                 MMU_CTRL_MEMTYPE_WB);
    if (vreg->kaddr == 0) {
        kfree(vreg);
        return NULL;
    }

    vreg->size = E2BITMAP_SIZE(count) * sizeof(bitmap_t);
#ifdef configVRALLOC_DEBUG
    vreg->magic = VREG_MAGIC_VALUE;
#endif

    LIST_INSERT_HEAD(&vrlist_head, vreg, _entry);

    /* Update stats */
    vralloc_all += VREG_BYTESIZE(count);

    return vreg;
}

/**
 * Get pcount number of unallocated pages.
 * @note needs to get vr_big_lock.
 * @param[out] iblock is the returned index of the allocation made.
 * @param pcount is the number of pages requested.
 * @return Returns a pointer to the allocated vreg.
 */
static struct vregion * get_iblocks(size_t * iblock, size_t pcount)
{
    struct vregion * vreg_temp;
    struct vregion * vreg = NULL;
    int err;

    mtx_lock(&vr_big_lock);

retry:
    LIST_FOREACH(vreg_temp, &vrlist_head, _entry) {
        if (bitmap_block_search(iblock, pcount, vreg_temp->map,
                                vreg_temp->size) == 0) {
            vreg = vreg_temp;
            break; /* Found a block */
        }
    }

    if (!vreg) { /* Not found */
        vreg = vreg_alloc_node(pcount);
        if (!vreg)
            goto out;
        goto retry;
    }

    err = bitmap_block_update(vreg->map, 1, *iblock, pcount, vreg->size);
    KASSERT(err == 0, "vreg map update OOB");
    vreg->count += pcount;
    vralloc_used += VREG_BYTESIZE(pcount);
out:
    mtx_unlock(&vr_big_lock);
    return vreg;
}

/**
 * vregion free callback.
 * This function is called by kobj.
 */
static void vreg_free_callback(struct kobj * obj)
{
    struct buf * bp = containerof(obj, struct buf, b_obj);
    struct vregion * vreg = (struct vregion *)(bp->allocator_data);
    const size_t bcount = VREG_PCOUNT(bp->b_bufsize);
    size_t iblock;
    int err;

    mtx_lock(&vr_big_lock);

#ifdef configVRALLOC_DEBUG
    KASSERT(vreg->magic == VREG_MAGIC_VALUE, "magic is correct");
#endif

    /* Get the iblock no. */
    iblock = VREG_ADDR2I(vreg, bp->b_data);

    err = bitmap_block_update(vreg->map, 0, iblock, bcount, vreg->size);
    KASSERT(err == 0, "vreg map update OOB");
    vreg->count -= bcount;

    vralloc_used -= bp->b_bufsize; /* Update stats */

    if (vreg->count == 0) { /* Free the vregion node */
        LIST_REMOVE(vreg, _entry);
        vralloc_all -= vreg->size * (4 * 8) * MMU_PGSIZE_COARSE;

        mtx_unlock(&vr_big_lock);

        dynmem_free_region((void *)vreg->kaddr);
        kfree(vreg);
    } else {
        mtx_unlock(&vr_big_lock);
    }

    kfree(bp);
}

struct buf * geteblk(size_t size)
{
    size_t iblock; /* Block index of the allocation */
    const size_t orig_size = size;
    size = memalign_size(size, MMU_PGSIZE_COARSE);
    const size_t pcount = VREG_PCOUNT(size);
    struct vregion * vreg;
    struct buf * bp;

    bp = kzalloc(sizeof(struct buf));
    if (!bp) {
        KERROR_DBG("%s: Can't allocate vm_region struct\n", __func__);
        return NULL;
    }

    vreg = get_iblocks(&iblock, pcount);
    if (!vreg) {
        KERROR_DBG("%s: Can't get vregion for a new buffer\n",
                   __func__);
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

    /* Clear allocated pages. */
    memset((void *)bp->b_data, 0, bp->b_bufsize);

    return bp;
}

/**
 * Increment reference count of a vr allocated vm_region.
 * @param region is a pointer to the vregion.
 */
static void vrref(struct buf * bp)
{
    if (kobj_ref(&bp->b_obj))
        panic("vrref error");
}

/**
 * Clone a vregion.
 * @param old_region is the old region to be cloned.
 * @return  Returns a pointer to the new vregion if operation was successful;
 *          Otherwise zero.
 */
static struct buf * vr_rclone(struct buf * old_region)
{
    struct buf * new_region;
    const size_t rsize = old_region->b_bufsize;

    new_region = geteblk(rsize);
    if (!new_region) {
        KERROR(KERROR_ERR, "%s: Out of memory, tried to allocate %d bytes\n",
               __func__, (unsigned)rsize);
        return NULL;
    }

    KERROR_DBG("clone %x -> %x, %u bytes\n",
               (unsigned)old_region->b_data, (unsigned)new_region->b_data,
               (unsigned)rsize);

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
    mtx_lock(&vr_big_lock);

    if (blockdiff > 0) {
        const size_t sblock = VREG_ADDR2I(vreg, bp->b_data) + bcount;

        if (bitmap_block_search_s(sblock, &iblock, blockdiff, vreg->map,
                    vreg->size) == 0) {
            if (iblock == sblock + 1) {
                int err;
                err = bitmap_block_update(vreg->map, 1, sblock, blockdiff,
                                          vreg->size);
                KASSERT(err == 0, "vreg map update OOB");
            } else { /* Must allocate a new region */
                struct vregion * nvreg;
                uintptr_t new_addr;
                int err;

                nvreg = get_iblocks(&iblock, pcount);
                if (!nvreg) {
                    /*
                     * It's not nice to panic here but we don't have any
                     * method to inform the caller about OOM.
                     */
                    /* TODO We should probably kill the caller */
                    panic("OOM during allocbuf()");
                }

                new_addr = VREG_I2ADDR(nvreg, iblock);
                memcpy((void *)(new_addr), (void *)(bp->b_data), bp->b_bufsize);

                bp->b_mmu.paddr = new_addr;
                bp->b_data = bp->b_mmu.paddr; /* Currently this way as
                                               * kernel space is 1:1 */
                bp->allocator_data = nvreg;

                /* Free blocks from old vreg */
                err = bitmap_block_update(vreg->map, 0, iblock, bcount,
                                          vreg->size);
                KASSERT(err == 0, "vreg map update OOB");
            }
        }
    } else {
#if 0 /*
       * We don't usually want to shrunk because it's hard to get memory back.
       */
        const size_t sblock = VREG_ADDR2I(vreg, bp->b_data) + bcount +
                              blockdiff;
        int err;

        err = bitmap_block_update(vreg->map, 0, sblock, -blockdiff, vreg->size);
        KASSERT(err == 0, "vreg map update OOB");
#endif
    }

    bp->b_bufsize = new_size;
    bp->b_bcount = orig_size;
    bp->b_mmu.num_pages = pcount;

    mtx_unlock(&bp->lock);
    mtx_unlock(&vr_big_lock);
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
         * Not a vregion, try to clone manually.
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
