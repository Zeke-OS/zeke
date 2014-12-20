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
#include <libkern.h>
#include <kinit.h>
#include <bitmap.h>
#include <dllist.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <kstring.h>
#include <errno.h>
#include <kerror.h>
#include <proc.h>
#include <sys/sysctl.h>
#include <vm/vm.h>
#include <buf.h>

/* TODO Locks to protect global data structures. */

/**
 * vralloc region struct.
 * Struct describing a single dynmem alloc block of vrallocated memory.
 */
struct vregion {
    llist_nodedsc_t node;
    intptr_t kaddr; /*!< Kernel address of the block allocated from dynmem. */
    size_t count;   /*!< Reserved pages count. */
    size_t size;    /*!< Size of allocation bitmap in bytes. */
    bitmap_t map[0]; /*!< Bitmap of reserved pages. */
};

#define VREG_SIZE(count) \
    (sizeof(struct vregion) + E2BITMAP_SIZE(count) * sizeof(bitmap_t))

#define VREG_PCOUNT(byte_size_) \
    ((byte_size_) / MMU_PGSIZE_COARSE)

#define VREG_BYTESIZE(pcount_) \
    ((pcount_) * MMU_PGSIZE_COARSE)

#define VREG_I2ADDR(vreg_, iblock_) \
    ((vreg_)->kaddr + VREG_BYTESIZE(iblock_))

static llist_t * vrlist; /*!< List of all allocations done by vralloc. */
static struct vregion * last_vreg; /*!< Last node that contained empty pages. */

static size_t vmem_all;
SYSCTL_UINT(_vm, OID_AUTO, vmem_all, CTLFLAG_RD, &vmem_all, 0,
    "Amount of memory currently allocated");

static size_t vmem_used;
SYSCTL_UINT(_vm, OID_AUTO, vmem_used, CTLFLAG_RD, &vmem_used, 0,
    "Amount of memory used");

static struct vregion * vreg_alloc_node(size_t count);
static void vreg_free_node(struct vregion * reg);
static int get_iblocks(size_t * iblock, size_t pcount,
                      struct vregion ** vreg_ret);
static void vrref(struct buf * region);

extern void _bio_init(void);

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
int vralloc_init(void) __attribute__((constructor));
int vralloc_init(void)
{
    SUBSYS_INIT("vrallloc");
    struct vregion * reg;

    vrlist = dllist_create(struct vregion, node);
    reg = vreg_alloc_node(256);
    if (!(vrlist && reg)) {
        panic("Can't initialize vralloc.");
    }

    last_vreg = reg;

    _bio_init();

    return 0;
}

/**
 * Allocate a new vregion node/chunk and memory for the region.
 * @param count is the page count (4kB pages). Should be multiple of 256.
 * @return Rerturns a pointer to the newly allocated region; Otherwise 0.
 */
static struct vregion * vreg_alloc_node(size_t count)
{
    struct vregion * vreg;

    vreg = kcalloc(1, VREG_SIZE(count));
    if (!vreg)
        return 0;

    vreg->kaddr = (intptr_t)dynmem_alloc_region(count / 256, MMU_AP_RWNA,
            MMU_CTRL_MEMTYPE_WB);
    if (vreg->kaddr == 0) {
        kfree(vreg);
        return 0;
    }

    vreg->size = E2BITMAP_SIZE(count) * sizeof(bitmap_t);
    vrlist->insert_head(vrlist, vreg);

    /* Update stats */
    vmem_all += VREG_BYTESIZE(count);

    return vreg;
}

/**
 * Free vregion node.
 * @param vreg is a pointer to vregion.
 */
static void vreg_free_node(struct vregion * vreg)
{
    /* Update stats */
    vmem_all -= vreg->size * (4 * 8) * MMU_PGSIZE_COARSE;

    /* Free node */
    vrlist->remove(vrlist, vreg);
    dynmem_free_region((void *)vreg->kaddr);
    kfree(vreg);
}

static int get_iblocks(size_t * iblock, size_t pcount,
                       struct vregion ** vreg_ret)
{
    struct vregion * vreg = last_vreg;

retry_vra:
    do {
        if (bitmap_block_search(iblock, pcount, vreg->map, vreg->size) == 0)
            break; /* Found block */
    } while ((vreg = vreg->node.next));

    if (!vreg) { /* Not found */
        vreg = vreg_alloc_node(pcount);
        if (!vreg)
            return -ENOMEM;
        goto retry_vra;
    }

    bitmap_block_update(vreg->map, 1, *iblock, pcount);
    *vreg_ret = vreg;

    return 0;
}

struct buf * geteblk(size_t size)
{
    size_t iblock; /* Block index of the allocation */
    const size_t orig_size = size;
    size = memalign_size(size, MMU_PGSIZE_COARSE);
    const size_t pcount = VREG_PCOUNT(size);
    struct vregion * vreg;
    struct buf * retval = NULL;

    if (get_iblocks(&iblock, pcount, &vreg))
        return NULL;

    retval = kcalloc(1, sizeof(struct buf));
    if (!retval)
        return 0; /* Can't allocate vm_region struct */
    mtx_init(&(retval->lock), MTX_TYPE_TICKET);

    /* Update target struct */
    retval->b_mmu.paddr = VREG_I2ADDR(vreg, iblock);
    retval->b_mmu.num_pages = pcount;
    retval->b_data = retval->b_mmu.paddr; /* Currently this way as
                                           * kernel space is 1:1 */
    retval->b_bufsize = VREG_BYTESIZE(pcount);
    retval->b_bcount = orig_size;
    retval->b_flags = B_BUSY;
    retval->refcount = 1;
    retval->allocator_data = vreg;
    retval->vm_ops = &vra_ops;
    retval->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
    vm_updateusr_ap(retval);

    vreg->count += pcount;

    /* Update stats */
    vmem_used += size;
    last_vreg = vreg;

    return retval;
}

/* TODO We may want to use bitmaps here */
mtx_t l_ksect_next;
static uintptr_t ksect_next = MMU_VADDR_KSECT_START;
static uintptr_t get_ksect_addr(size_t region_size)
{
    uintptr_t retval;

    if (l_ksect_next.mtx_tflags == MTX_TYPE_UNDEF)
        mtx_init(&l_ksect_next, MTX_TYPE_SPIN);

    mtx_lock(&l_ksect_next);

    if (region_size < MMU_PGSIZE_SECTION)
        retval = memalign_size(ksect_next, MMU_PGSIZE_SECTION);
    else
        retval = memalign_size(ksect_next, MMU_PGSIZE_COARSE);

    if (retval > MMU_VADDR_KSECT_END)
        return 0;

    ksect_next += retval;

    mtx_unlock(&l_ksect_next);

    return retval;
}

struct buf * geteblk_special(size_t size, uint32_t control)
{
    struct proc_info * p = proc_get_struct_l(0);
    const uintptr_t kvaddr = get_ksect_addr(size);
    struct vm_pt * vpt;
    struct buf * buf;

    KASSERT(p, "Can't get the PCB of pid 0");

    if (kvaddr == 0)
        return NULL;

    buf = proc_newsect(kvaddr, size, VM_PROT_READ | VM_PROT_WRITE);
    if (!buf)
        return NULL;

    buf->b_mmu.control = control;

    vpt = ptlist_get_pt(&p->mm.ptlist_head, &p->mm.mpt,
                        buf->b_mmu.vaddr);
    if (!vpt) {
        panic("Mapping a kernel special buffer failed");
    }

    buf->b_mmu.pt = &vpt->pt;
    vm_map_region(buf, vpt);
    vm_add_region(&p->mm, buf);

    buf->b_data = buf->b_mmu.vaddr; /* Should be same as kvaddr */

    return buf;
}

/**
 * Increment reference count of a vr allocated vm_region.
 * @param region is a vregion.
 */
static void vrref(struct buf * region)
{
    mtx_lock(&(region->lock));
    region->refcount++;
    mtx_unlock(&(region->lock));
}

struct buf * vr_rclone(struct buf * old_region)
{
    struct buf * new_region;
    const size_t rsize = old_region->b_bufsize;

    /* "Lock", ensure that the region is not freed during the operation. */
    vrref(old_region);

    new_region = geteblk(rsize);
    if (!new_region) {
        KERROR(KERROR_ERR, "Out of memory\n");
        return NULL;
    }

#if configDEBUG >= KERROR_DEBUG
    {
    char buf[80];
    ksprintf(buf, sizeof(buf), "clone %x -> %x, %u bytes\n",
            old_region->b_data, new_region->b_data, rsize);
    KERROR(KERROR_DEBUG, buf);
    }
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

    if (!vreg)
        panic("bp->allocator_data not set");

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

                if (get_iblocks(&iblock, pcount, &nvreg))
                    panic("OOM during allocbuf()");

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
#if 0 /* We don't usually want to shrunk because it's hard to get memory back. */
        const size_t sblock = VREG_PCOUNT((bp->b_data - vreg->kaddr)) + bcount + blockdiff;

        bitmap_block_update(vreg->map, 0, sblock, -blockdiff);
#endif
    }

    vreg->count += blockdiff;
    bp->b_bufsize = new_size;
    bp->b_bcount = orig_size;
    bp->b_mmu.num_pages = pcount;

    mtx_unlock(&bp->lock);
}

void vrfree(struct buf * region)
{
    struct vregion * vreg;
    size_t iblock;
    const size_t bcount = VREG_PCOUNT(region->b_bufsize);

    mtx_lock(&(region->lock));
    region->refcount--;
    if (region->refcount > 0) {
        mtx_unlock(&(region->lock));
        return;
    }
    mtx_unlock(&(region->lock));

    vreg = (struct vregion *)(region->allocator_data);
    iblock = VREG_PCOUNT(region->b_data - vreg->kaddr);

    bitmap_block_update(vreg->map, 0, iblock, bcount);
    vreg->count -= bcount;
    vmem_used -= region->b_bufsize; /* Update stats */

    kfree(region);

    if (vreg->count <= 0 && last_vreg != vreg) {
        vreg_free_node(vreg);
    }
}
