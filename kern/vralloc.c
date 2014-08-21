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
#include <bitmap.h>
#include <dllist.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <kstring.h>
#include <kerror.h>
#include <sys/sysctl.h>
#include <vm/vm.h>
#include <buf.h>

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
static size_t pagealign(size_t size, size_t bytes);
static void vrref(struct buf * region);

extern void _add2bioman(struct buf * bp);

/**
 * VRA specific operations for allocated vm regions.
 */
static const vm_ops_t vra_ops = {
    .rref = vrref,
    .rclone = vr_rclone,
    .rfree = brelse
};


/**
 * Initializes vregion allocator data structures.
 */
void vralloc_init(void) __attribute__((constructor));
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

    SUBSYS_INITFINI("vralloc OK");
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
    vmem_all += count * MMU_PGSIZE_COARSE;

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

struct buf * geteblk(size_t size)
{
    size_t iblock; /* Block index of the allocation */
    const size_t orig_size = size;
    size = pagealign(size, MMU_PGSIZE_COARSE);
    const size_t pcount = size / MMU_PGSIZE_COARSE;
    struct vregion * vreg = last_vreg;
    struct buf * retval = 0;

retry_vra:
    do {
        if (bitmap_block_search(&iblock, pcount, vreg->map, vreg->size) == 0) {
            /* Found block */
            retval = kcalloc(1, sizeof(struct buf));
            if (!retval)
                return 0; /* Can't allocate vm_region struct */
            mtx_init(&(retval->lock), MTX_TYPE_SPIN);

            /* Update target struct */
            retval->b_mmu.paddr = vreg->kaddr + iblock * MMU_PGSIZE_COARSE;
            retval->b_mmu.num_pages = pcount;
            retval->b_data = retval->b_mmu.paddr; /* Currently this way as
                                                   * kernel space is 1:1 */
            retval->b_bufsize = pcount * MMU_PGSIZE_COARSE;
            retval->b_bcount = orig_size;
            retval->refcount = 1;
            retval->allocator_data = vreg;
            retval->vm_ops = &vra_ops;
            retval->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
            vm_updateusr_ap(retval);

            vreg->count += pcount;
            bitmap_block_update(vreg->map, 1, iblock, pcount);

            break;
        }
    } while ((vreg = vreg->node.next));
    if (retval == 0) { /* Not found */
        vreg = vreg_alloc_node(pagealign(size, MMU_PGSIZE_SECTION) / MMU_PGSIZE_COARSE);
        if (!vreg)
            return 0;
        goto retry_vra;
    }

    vmem_used += size; /* Update stats */
    last_vreg = vreg;

    _add2bioman(retval);

    return retval;
}

/**
 * Increment reference count of a vr allocated vm_region.
 * @param region is a vregion.
 */
static void vrref(struct buf * region)
{
    mtx_spinlock(&(region->lock));
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
        KERROR(KERROR_ERR, "Out of memory");
        return 0;
    }

#if configDEBUG >= KERROR_DEBUG
    {
    char buf[80];
    ksprintf(buf, sizeof(buf), "clone %x -> %x, %u bytes",
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
    brelse(old_region);

    return new_region;
}

void allocbuf(struct buf * bp, size_t size)
{
    /* TODO */
}

void brelse(struct buf * region)
{
    struct vregion * vreg;
    size_t iblock;

    mtx_spinlock(&(region->lock));
    region->refcount--;
    if (region->refcount > 0) {
        mtx_unlock(&(region->lock));
        return;
    }
    mtx_unlock(&(region->lock));

    vreg = (struct vregion *)(region->allocator_data);
    iblock = (region->b_data - vreg->kaddr) / MMU_PGSIZE_COARSE;

    bitmap_block_update(vreg->map, 0, iblock, region->b_bufsize / MMU_PGSIZE_COARSE);
    vreg->count -= region->b_bufsize * MMU_PGSIZE_COARSE;
    vmem_used -= region->b_bufsize; /* Update stats */

    kfree(region);

    if (vreg->count <= 0 && last_vreg != vreg) {
        vreg_free_node(vreg);
    }
}
