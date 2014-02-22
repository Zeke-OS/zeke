/**
 *******************************************************************************
 * @file    vralloc.c
 * @author  Olli Vanhoja
 * @brief   Virtual Region Allocator.
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
#include <kinit.h>
#include <generic/bitmap.h>
#include <generic/dllist.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <kerror.h>
#include <vralloc.h>

struct vregion {
    llist_nodedsc_t node;
    intptr_t paddr; /*!< Physical address allocated from dynmem. */
    size_t count; /*!< Count of reserved pages */
    size_t size; /*!< Size of bitmap in bytes. */
    bitmap_t map[0]; /*!< Bitmap of reserved pages. */
};

#define VREG_SIZE(count) (sizeof(struct vregion) + E2BITMAP_SIZE(count))

static llist_t * vrlist; /*!< List of all allocations. */
static struct vregion * last_vreg; /*!< Last node that contained empty pages. */

static struct vregion * vreg_alloc_node(size_t count);
static void vreg_free_node(struct vregion * reg);
static size_t pagealign(size_t size, size_t bytes);
static void vrref(struct vm_region * region);

static vm_ops_t vra_ops = {
    .rref = vrref, /* TODO */
    .rclone = 0, /* TODO */
    .rfree = vrfree
};


void vralloc_init(void)
{
    SUBSYS_INIT();
    struct vregion * reg;

    vrlist = dllist_create(struct vregion, node);
    reg = vreg_alloc_node(256);
    if (!(vrlist && reg)) {
        panic("Can't initialize vralloc.");
    }

    last_vreg = reg;

    SUBSYS_INITFINI("vralloc init");
}

/**
 * Allocate a new vregion node and memory for the region.
 * @param count is the page count (4kB pages). Should be multiple of 256.
 * @return Rerturns a pointer to the newly allocated region; Otherwise 0.
 */
static struct vregion * vreg_alloc_node(size_t count)
{
    struct vregion * vreg;

    vreg = kcalloc(1, VREG_SIZE(count));
    if (!vreg)
        return 0;

    vreg->paddr = (intptr_t)dynmem_alloc_region(count / 256, MMU_AP_RWNA,
            MMU_CTRL_MEMTYPE_WB);
    if (vreg->paddr == 0) {
        kfree(vreg);
        return 0;
    }

    vreg->size = VREG_SIZE(count) * sizeof(uint32_t);
    vrlist->insert_head(vrlist, vreg);

    return vreg;
}

/**
 * Free allocated vregion.
 */
static void vreg_free_node(struct vregion * vreg)
{
    vrlist->remove(vrlist, vreg);
    dynmem_free_region((void *)vreg->paddr);
    kfree(vreg);
}

/**
 * Return page aligned size of size.
 * @param size is a size of a memory block requested.
 * @param bytes align to bytes.
 * @returns Returns size aligned to the word size of the current system.
 */
static size_t pagealign(size_t size, size_t bytes)
{
#define MOD_AL(x) ((x) & (bytes - 1)) /* x % bytes */
    size_t padding = MOD_AL((bytes - (MOD_AL(size))));

    return size + padding;
#undef MOD_AL
}

/**
 * Allocate vregion.
 * @param size is the size of new region in bytes.
 */
vm_region_t * vralloc(size_t size)
{
    size_t iblock;
    size = pagealign(size, 4096);
    const size_t pcount = size / 4096;
    struct vregion * vreg = last_vreg;
    vm_region_t * retval = 0;

retry_vra:
    do {
        if(!bitmap_block_search(&iblock, pcount, vreg->map, vreg->size)) {
            /* Found block */
            retval = kcalloc(1, sizeof(vm_region_t));
            if (!retval)
                return 0; /* Can't allocate vm_region struct */
            mtx_init(&(retval->lock), MTX_DEF | MTX_SPIN);

            /* Update target struct */
            retval->mmu.paddr = vreg->paddr + iblock * 4096;
            retval->mmu.num_pages = pcount;
#if configDEBUG != 0
            retval->allocator_id = VRALLOC_ALLOCATOR_ID;
#endif
            retval->refcount = 1;
            retval->allocator_data = vreg;
            retval->vm_ops = &vra_ops;

            vreg->count += pcount;
            bitmap_block_update(vreg->map, 1, iblock, pcount);

            break;
        }
    } while ((vreg = vreg->node.next));
    if (retval == 0) { /* Not found */
        vreg = vreg_alloc_node(pagealign(size, 1048576) / 4096);
        if (!vreg)
            return 0;
        goto retry_vra;
    }

    last_vreg = vreg;
    return retval;
}

static void vrref(struct vm_region * region)
{
    mtx_spinlock(&(region->lock));
    region->refcount++;
    mtx_unlock(&(region->lock));
}

void vrfree(struct vm_region * region)
{
    struct vregion * vreg;
    size_t iblock;

#if configDEBUG != 0
    if (region->allocator_id != VRALLOC_ALLOCATOR_ID) {
        KERROR(KERROR_ERR, "Invalid allocator_id");
        return;
    }
#endif

    mtx_spinlock(&(region->lock));
    region->refcount--;
    if (region->refcount > 0) {
        mtx_unlock(&(region->lock));
        return;
    }
    mtx_unlock(&(region->lock));

    vreg = (struct vregion *)(region->allocator_data);
    iblock = (region->mmu.paddr - vreg->paddr) / 4096;

    bitmap_block_update(vreg->map, 0, iblock, region->mmu.num_pages);
    vreg->count -= region->mmu.num_pages;

    kfree(region);

    if (vreg->count <= 0) {
        if (last_vreg != vreg)
            vreg_free_node(vreg);
    }
}
