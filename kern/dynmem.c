/**
 *******************************************************************************
 * @file    dynmem.c
 * @author  Olli Vanhoja
 * @brief   Dynmem management.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <dynmem.h>
#include <hal/sysinfo.h>
#include <kerror.h>
#include <kinit.h>
#include <klocks.h>
#include <kmem.h>
#include <kstring.h>

/**
 * Dynmem area starts.
 */
#define DYNMEM_START        configDYNMEM_START

/**
 * Dynmem area end.
 * Temporarily set to a safe value and overriden on init.
 */
static size_t dynmem_end = (DYNMEM_START + configDYNMEM_SAFE_SIZE - 1);

/**
 * Size of dynmem page table in pt region.
 */
#define DYNMEM_PT_SIZE      MMU_PTSZ_COARSE

/*
 * Region Link bits
 */
/** No Link */
#define DYNMEM_RL_NIL       0x0
/** Link */
#define DYNMEM_RL_LINK      0x1

/**
 * Dynmemmap size.
 * Dynmem memory space is allocated in 1MB sections.
 */
#define DYNMEM_MAPSIZE  ((configDYNMEM_MAX_SIZE) / DYNMEM_PAGE_SIZE)

#if E2BITMAP_SIZE(DYNMEM_MAPSIZE) > 0
#define DYNMEM_BITMAPSIZE       E2BITMAP_SIZE(DYNMEM_MAPSIZE)
#else
#define DYNMEM_BITMAPSIZE       1
#endif

#define SIZEOF_DYNMEMMAP        (DYNMEM_MAPSIZE * sizeof(uint32_t))
#define SIZEOF_DYNMEMMAP_BITMAP (DYNMEM_BITMAPSIZE * sizeof(uint32_t))

struct dynmem_desc {
    unsigned control    : 10;
    unsigned ap         : 3;
    unsigned rl         : 1;
    unsigned _padding   : 2;
    unsigned refcount   : 16;
};

/**
 * Dynmemmap allocation table.
 */
static struct dynmem_desc dynmemmap[DYNMEM_MAPSIZE];
static uint32_t dynmemmap_bitmap[DYNMEM_BITMAPSIZE];

/**
 * Struct for temporary storage.
 */
static mmu_region_t dynmem_region;

/**
 * Lock used to protect dynmem_region struct, dynmem_bitmap and dynmemmap
 * access.
 */
static mtx_t dynmem_region_lock;

/*
 * Memory areas reserved for some other use and shall not be touched by dynmem.
 */
SET_DECLARE(dynmem_reserved, struct dynmem_reserved_area);

/*
 * Some sysctl stat variables.
 */
static size_t dynmem_free = configDYNMEM_SAFE_SIZE;
SYSCTL_UINT(_vm, OID_AUTO, dynmem_free, CTLFLAG_RD, (void *)(&dynmem_free), 0,
        "Amount of free dynmem");

static const size_t dynmem_tot = configDYNMEM_SAFE_SIZE;
SYSCTL_UINT(_vm, OID_AUTO, dynmem_tot, CTLFLAG_RD, (void *)(&dynmem_tot), 0,
        "Total amount of dynmem");

static void mark_reserved_areas(void)
{
    struct dynmem_reserved_area ** areap;

    SET_FOREACH(areap, dynmem_reserved) {
        struct dynmem_reserved_area * area = *areap;
        const size_t pos = (size_t)area->caddr_start - DYNMEM_START;
        uintptr_t end_addr;
        size_t blkcount;

        if (area->caddr_start > dynmem_end)
            continue;

        end_addr = (area->caddr_end > dynmem_end) ? dynmem_end :
                                                    area->caddr_end;
        blkcount = (end_addr - area->caddr_start + 1) / DYNMEM_PAGE_SIZE;
        bitmap_block_update(dynmemmap_bitmap, 1, pos, blkcount);
    }
}

static int dynmem_init(void)
{
    SUBSYS_DEP(mmu_init);
    SUBSYS_INIT("dynmem");

    /*
     * Set dynmem end address.
     */
    if (sysinfo.mem.size > configDYNMEM_MAX_SIZE)
        dynmem_end = sysinfo.mem.start + configDYNMEM_MAX_SIZE - 1;
    else
        dynmem_end = sysinfo.mem.start + sysinfo.mem.size - 1;

    mark_reserved_areas();

    mtx_init(&dynmem_region_lock, MTX_TYPE_SPIN, 0);

    return 0;
}
HW_PREINIT_ENTRY(dynmem_init);

/**
 * Check that the given address is in dynmem range.
 * @param addr  is the address of the dynmem allocation.
 * @param test  if 1 tests if actually allocated;
 *              Otherwise only tests validity of addr.
 * @return Boolean true if valid; Otherwise false/0.
 */
static int addr_is_valid(const void * addr, int test)
{
    const size_t i = (size_t)addr - DYNMEM_START;

    KASSERT(mtx_test(&dynmem_region_lock), "dynmem must be locked");

    if ((size_t)addr < DYNMEM_START || (size_t)addr > dynmem_end)
        return 0; /* Not in range */

    if (test && dynmemmap[i].refcount == 0)
        return 0; /* Not allocated */

    return (1 == 1);
}

/**
 * Update the region strucure to describe a already allocated dynmem region.
 * Updates dynmem_region structures.
 * @note dynmem_region_lock must be held before calling this function.
 * @param p Pointer to the begining of the allocated dynmem section (physical).
 * @return  0 or negative errno code.
 */
static int update_dynmem_region_struct(void * base)
{
    size_t reg_start = (size_t)base - DYNMEM_START;
    uint32_t num_pages;
    struct dynmem_desc * dp;
    struct dynmem_desc flags;

    KASSERT(mtx_test(&dynmem_region_lock), "dynmem must be locked");

#ifdef configDYNEM_DEBUG
    if (!addr_is_valid(base, 1)) {
        KERROR(KERROR_ERR, "%s(base %p): Invalid dynmem region addr\n",
               __func__, base);

        return -EINVAL;
    }
#endif

    num_pages = 1;
    dp = dynmemmap + reg_start;
    while (dp->rl == DYNMEM_RL_LINK) {
        dp++;
        num_pages++;
    }
    flags = dynmemmap[reg_start];

    dynmem_region.vaddr = (uintptr_t)base; /* 1:1 mapping by default */
    dynmem_region.paddr = (uintptr_t)base;
    dynmem_region.num_pages = num_pages;
    dynmem_region.ap = flags.ap;
    dynmem_region.control = flags.control;
    dynmem_region.pt = &mmu_pagetable_master;

    return 0;
}

/**
 * Updates dynmem allocation table and intially maps the memory region to the
 * kernel memory space.
 * @note dynmem_region_lock must be held before entering this function.
 * @param base      is the base address index.
 * @param size      is the size of the memory region in MB.
 * @param ap        Access Permissions.
 * @param ctrl      Control bits.
 */
static void * kmap_allocation(size_t base, size_t size, uint32_t ap,
                              uint32_t ctrl)
{
    size_t i;
    const size_t addr = DYNMEM_START + base * DYNMEM_PAGE_SIZE;
    const struct dynmem_desc desc = {
        .control = ctrl,
        .ap = ap,
        .refcount = 1,
    };

    KASSERT(mtx_test(&dynmem_region_lock), "dynmem must be locked");

    for (i = base; i < base + size - 1; i++) {
        struct dynmem_desc * dp = dynmemmap + i;

        *dp = desc;
        dp->rl = DYNMEM_RL_LINK;
    }
    dynmemmap[i] = desc;
    dynmemmap[i].rl = DYNMEM_RL_NIL;

    /* Map the memory region to the kernel memory space */
    dynmem_region.vaddr = addr;
    dynmem_region.paddr = addr;
    dynmem_region.num_pages = size;
    dynmem_region.ap = ap;
    dynmem_region.control = ctrl;
    dynmem_region.pt = &mmu_pagetable_master;
    mmu_map_region(&dynmem_region);

    return (void *)addr;
}

void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t ctrl)
{
    size_t pos;
    void * retval = NULL;

    if (size == 0)
        return NULL;

    mtx_lock(&dynmem_region_lock);

    if (bitmap_block_search(&pos, size, dynmemmap_bitmap,
                           SIZEOF_DYNMEMMAP_BITMAP)) {
        KERROR(KERROR_ERR, "%s(size %u): Out of dynmem, free %u/%u\n",
               __func__, size, dynmem_free, dynmem_tot);
        goto out;
    }

    /* Update sysctl stats */
    dynmem_free -= size * DYNMEM_PAGE_SIZE;

    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    retval = kmap_allocation(pos, size, ap, ctrl);

out:
    mtx_unlock(&dynmem_region_lock);
    return retval;
}

void * dynmem_alloc_force(void * addr, size_t size, uint32_t ap, uint32_t ctrl)
{
    size_t pos = (size_t)addr - DYNMEM_START;
    void * retval = NULL;

    if (size == 0)
        return NULL;

    mtx_lock(&dynmem_region_lock);

    if (!addr_is_valid(addr, 0)) {
        KERROR(KERROR_ERR, "%s(): Invalid address; %p\n", __func__, addr);

        goto out;
    }

    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    retval = kmap_allocation(pos, size, ap, ctrl);

out:
    mtx_unlock(&dynmem_region_lock);
    return retval;
}

int dynmem_ref(void * addr)
{
    size_t i = (size_t)addr - DYNMEM_START;
    int retval;

    mtx_lock(&dynmem_region_lock);

    if (!addr_is_valid(addr, 1)) {
        KERROR(KERROR_ERR, "%s(addr %p): Invalid address\n", __func__, addr);

        retval = -EINVAL;
        goto out;
    }
    if (dynmemmap[i].refcount == 0) {
        retval = -EINVAL;
        goto out;
    }

    dynmemmap[i].refcount++;

    retval = 0;
out:
    mtx_unlock(&dynmem_region_lock);
    return retval;
}

void dynmem_free_region(void * addr)
{
    struct dynmem_desc * dp;
    size_t i;

    mtx_lock(&dynmem_region_lock);

    if (!addr_is_valid(addr, 1)) {
        KERROR(KERROR_ERR, "%s(addr %p): Invalid address\n", __func__, addr);

        goto out;
    }

    i = (size_t)addr - DYNMEM_START;
    dp = dynmemmap + i;

    /* Check if there is any references */
    if (dp->refcount > 1) {
        dp->refcount--;
        goto out; /* Do not free yet. */
    }

    if (update_dynmem_region_struct(addr)) { /* error */
        KERROR(KERROR_ERR, "%s(addr %p): Can't free dynmem region\n",
               __func__, addr);

        goto out;
    }

    mmu_unmap_region(&dynmem_region);

    /* Mark the region as unused. */
    memset(dp, 0, dynmem_region.num_pages * sizeof(struct dynmem_desc));
    bitmap_block_update(dynmemmap_bitmap, 1, i, dynmem_region.num_pages);

    /* Update sysctl stats */
    dynmem_free += dynmem_region.num_pages * DYNMEM_PAGE_SIZE;

out:
    mtx_unlock(&dynmem_region_lock);
}

void * dynmem_clone(void * addr)
{
    mmu_region_t cln;
    void * new_region;

    /*
     * Take a reference to protect clone operation from concurrent
     * dynmem_free_region() call.
     */
    if (dynmem_ref(addr)) {
#ifdef configDYNEM_DEBUG
        KERROR(KERROR_ERR, "%s(addr %p): Can't clone given dynmem area\n",
               __func__, addr);
#endif
        return NULL;
    }

    mtx_lock(&dynmem_region_lock);

    /* Take a copy of dynmem_region struct for this region. */
    if (update_dynmem_region_struct(addr)) { /* error */
#ifdef configDYNEM_DEBUG
        KERROR(KERROR_ERR, "%s(addr %p): Clone failed\n", __func__, addr);
#endif
        mtx_unlock(&dynmem_region_lock);

        return NULL;
    }
    cln = dynmem_region;
    mtx_unlock(&dynmem_region_lock);

    /* Allocate a new region. */
    new_region = dynmem_alloc_region(cln.num_pages, cln.ap, cln.control);
    if (new_region == NULL) {
#ifdef configDYNEM_DEBUG
        KERROR(KERROR_ERR, "%s(addr %p): Out of dynmem while cloning\n",
                __func__, addr);
#endif
        return NULL;
    }

    /*
     * NOTE: We don't need lock here as dynmem_ref guarantees that the region
     * won't be removed during copy operation.
     */
    memcpy(new_region, (void *)(cln.paddr), cln.num_pages * DYNMEM_PAGE_SIZE);

    dynmem_free_region(addr);

    return new_region;
}

uint32_t dynmem_acc(const void * addr, size_t len)
{
    size_t size;
    uint32_t retval = 0;

    mtx_lock(&dynmem_region_lock);

    if (!addr_is_valid(addr, 1))
        goto out; /* Address out of bounds. */

    if (update_dynmem_region_struct((void *)addr)) { /* error */
#ifdef configDYNEM_DEBUG
        KERROR(KERROR_DEBUG, "%s(addr %p, len %p): Access check failed\n",
               __func__, addr, len);
#endif
        goto out;
    }

    /* Get size of the region */
    if (!(size = mmu_sizeof_region(&dynmem_region))) {
        KERROR(KERROR_WARN, "Possible dynmem corruption at %p\n", addr);

        goto out; /* Error in size calculation. */
    }
    if ((size_t)addr < dynmem_region.paddr ||
        (size_t)addr > (dynmem_region.paddr + size)) {
        goto out; /* Not in region range. */
    }

    /*
     * Access seems to be ok.
     * Calc ap + xn as a return value for further testing.
     */
    retval  = dynmem_region.ap;
    retval |= ((dynmem_region.control & MMU_CTRL_XN) == MMU_CTRL_XN) ?
              DYNMEM_XN : 0;

out:
    mtx_unlock(&dynmem_region_lock);
    return retval;
}
