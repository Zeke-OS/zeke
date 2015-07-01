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

#include <kinit.h>
#include <kstring.h>
#include <kerror.h>
#include <klocks.h>
#include <sys/sysctl.h>
#include <bitmap.h>
#include <hal/sysinfo.h>
#include <hal/mmu.h>
#include <ptmapper.h>
#include <dynmem.h>

/**
 * Dynmem page/region size in bytes.
 * In practice this should be always 1 MB.
 */
#define DYNMEM_PAGE_SIZE    MMU_PGSIZE_SECTION

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
#define DYNMEM_RL_NL        0x0
/** Begin Link/Continue Link */
#define DYNMEM_RL_BL        0x1
/** End Link */
#define DYNMEM_RL_EL        0x2

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
    unsigned rl         : 2;
    unsigned _padding   : 1;
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

static void mark_reserved_areas(void);
static void * kmap_allocation(size_t pos, size_t size, uint32_t ap,
                              uint32_t ctrl);
static int update_dynmem_region_struct(void * p);

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
 * @param addr  is the address.
 * @param test  if 1 tests if actually allocted; Otherwise only tests validity
 *              of addr.
 * @return Boolean true if valid; Otherwise false/0.
 */
static int validate_addr(const void * addr, int test)
{
    const size_t i = (size_t)addr - DYNMEM_START;

    if ((size_t)addr < DYNMEM_START || (size_t)addr > dynmem_end)
        return 0; /* Not in range */

    if (test && dynmemmap[i].refcount == 0)
        return 0; /* Not allocated */

    return (1 == 1);
}

static void mark_reserved_areas(void)
{
    struct dynmem_reserved_area ** areap;

    SET_FOREACH(areap, dynmem_reserved) {
        struct dynmem_reserved_area * area = *areap;
        const uint32_t pos = (uint32_t)area->caddr_start - DYNMEM_START;
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

void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t ctrl)
{
    uint32_t pos;
    void * retval = 0;

    mtx_lock(&dynmem_region_lock);

    if (bitmap_block_search(&pos, size, dynmemmap_bitmap,
                           SIZEOF_DYNMEMMAP_BITMAP)) {
        KERROR(KERROR_ERR,
               "Out of dynmem, free %u/%u, tried to allocate %u MB\n",
               dynmem_free, dynmem_tot, size);
        goto out;
    }

    /* Update sysctl stats */
    dynmem_free -= size * DYNMEM_PAGE_SIZE;

    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    retval = kmap_allocation((size_t)pos, size, ap, ctrl);

out:
    mtx_unlock(&dynmem_region_lock);
    return retval;
}

void * dynmem_alloc_force(void * addr, size_t size, uint32_t ap, uint32_t ctrl)
{
    size_t pos = (size_t)addr - DYNMEM_START;
    void * retval;

    if (!validate_addr(addr, 0)) {
        KERROR(KERROR_ERR, "Invalid address; dynmem_alloc_force(): %x\n",
               (uint32_t)addr);

        return 0;
    }

    mtx_lock(&dynmem_region_lock);
    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    retval = kmap_allocation(pos, size, ap, ctrl);
    mtx_unlock(&dynmem_region_lock);

    return retval;
}

void * dynmem_ref(void * addr)
{
    size_t i = (size_t)addr - DYNMEM_START;

    if (!validate_addr(addr, 1)) {
        KERROR(KERROR_ERR, "Invalid address: %x\n", (uint32_t)addr);

        return 0;
    }
    if (dynmemmap[i].refcount == 0)
        return 0;

    mtx_lock(&dynmem_region_lock);
    dynmemmap[i].refcount++;
    mtx_unlock(&dynmem_region_lock);

    return addr;
}

void dynmem_free_region(void * addr)
{
    uint32_t i, j, rc;

    if (!validate_addr(addr, 1)) {
        KERROR(KERROR_ERR, "Invalid address: %x\n", (uint32_t)addr);

        return;
    }

    i = (uint32_t)addr - DYNMEM_START;
    rc = dynmemmap[i].refcount;

    /* Get lock to dynmem_region */
    mtx_lock(&dynmem_region_lock);

    /* Check if there is any references */
    if (rc > 1) {
        dynmemmap[i].refcount--;
        goto out; /* Do not free yet. */
    }

    if (update_dynmem_region_struct(addr)) { /* error */
        KERROR(KERROR_ERR, "Can't free dynmem region: %x\n", (uint32_t)addr);

        goto out;
    }

    mmu_unmap_region(&dynmem_region);

    /* Mark the region as unused. */
    for (j = i; j < dynmem_region.num_pages; j++) {
        memset(&dynmemmap[j], 0, sizeof(struct dynmem_desc));
    }
    bitmap_block_update(dynmemmap_bitmap, 1, j, dynmem_region.num_pages);

    /* Update sysctl stats */
    dynmem_free += dynmem_region.num_pages * DYNMEM_PAGE_SIZE;

out:
    mtx_unlock(&dynmem_region_lock);
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
    const uint32_t rlb = (size > 1) ? DYNMEM_RL_BL : DYNMEM_RL_NL;
    const uint32_t rle = (size > 1) ? DYNMEM_RL_EL : DYNMEM_RL_NL;
    const uint32_t addr = DYNMEM_START + base * DYNMEM_PAGE_SIZE;
    const struct dynmem_desc desc = {
        .control = ctrl,
        .ap = ap,
        .refcount = 1,
    };

    for (i = base; i < base + size - 1; i++) {
        dynmemmap[i] = desc;
        dynmemmap[i].rl = rlb;
    }
    dynmemmap[i] = desc;
    dynmemmap[i].rl = rle;

    /* Map memory region to the kernel memory space */
    dynmem_region.vaddr = addr;
    dynmem_region.paddr = addr;
    dynmem_region.num_pages = size;
    dynmem_region.ap = ap;
    dynmem_region.control = ctrl;
    dynmem_region.pt = &mmu_pagetable_master;
    mmu_map_region(&dynmem_region);

    return (void *)addr;
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
    uint32_t i;
    uint32_t reg_start = (uint32_t)base - DYNMEM_START;
    uint32_t reg_end;
    struct dynmem_desc flags;

#if configDEBUG >= KERROR_ERR
    if (!validate_addr(base, 1)) {
        KERROR(KERROR_ERR, "Invalid dynmem region addr: %x\n", (uint32_t)base);

        return -EINVAL;
    }
#endif

    i = reg_start;
    if (dynmemmap[i].rl == DYNMEM_RL_BL) {
        /* Region linking begins */
        while (++i <= dynmem_end) {
            if (!(dynmemmap[i].rl == DYNMEM_RL_EL)) {
                /* This was the last entry of this region */
                i--;
                break;
            }
        }
    } /* else this was the only section in this region. */
    reg_end = i;
    flags = dynmemmap[reg_start];

    dynmem_region.vaddr = (uint32_t)base; /* 1:1 mapping by default */
    dynmem_region.paddr = (uint32_t)base;
    dynmem_region.num_pages = reg_end - reg_start + 1;
    dynmem_region.ap = flags.ap;
    dynmem_region.control = flags.control;
    dynmem_region.pt = &mmu_pagetable_master;

    return 0;
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
#if configDEBUG >= KERROR_ERR
        KERROR(KERROR_ERR, "Can't clone given dynmem area @ %x\n",
               (uint32_t)addr);
#endif
        return 0;
    }

    /* Take a copy of dynmem_region struct for this region. */
    mtx_lock(&dynmem_region_lock);
    if (update_dynmem_region_struct(addr)) { /* error */
#if configDEBUG  >= KERROR_ERR
        KERROR(KERROR_ERR, "Clone failed @ %x\n", (uint32_t)addr);
#endif
        mtx_unlock(&dynmem_region_lock);
        return 0;
    }
    cln = dynmem_region;
    mtx_unlock(&dynmem_region_lock);

    /* Allocate a new region. */
    new_region = dynmem_alloc_region(cln.num_pages, cln.ap, cln.control);
    if (new_region == 0) {
#if configDEBUG >= KERROR_ERR
        KERROR(KERROR_ERR, "Out of dynmem while cloning @ %x",
               (uint32_t)addr);
#endif
        return 0;
    }

    /*
     * NOTE: We don't need lock here as dynmem_ref quarantees that the region is
     * not removed during copy operation.
     */
    memcpy(new_region, (void *)(cln.paddr), cln.num_pages * DYNMEM_PAGE_SIZE);

    dynmem_free_region(addr);

    return new_region;
}

uint32_t dynmem_acc(const void * addr, size_t len)
{
    size_t size;
    uint32_t retval = 0;

    if (!validate_addr(addr, 1))
        goto out; /* Address out of bounds. */

    mtx_lock(&dynmem_region_lock);
    if (update_dynmem_region_struct((void *)addr)) { /* error */
#if configDEBUG >= KERROR_ERR
        KERROR(KERROR_DEBUG, "dynmem_acc() check failed for: %x\n",
            (uint32_t)addr);
#endif
        goto out;
    }

    /* Get size */
    if (!(size = mmu_sizeof_region(&dynmem_region))) {
#if configDEBUG >= KERROR_WARN
        KERROR(KERROR_ERR, "Possible dynmem corruption at: %x\n",
                (uint32_t)addr);
#endif
        goto out; /* Error in size calculation. */
    }
    if ((size_t)addr < dynmem_region.paddr
            || (size_t)addr > (dynmem_region.paddr + size)) {
        goto out; /* Not in region range. */
    }

    /*
     * Access seems to be ok.
     * Calc ap + xn as a return value for further testing.
     */
    retval = dynmem_region.ap |
        (((dynmem_region.control & MMU_CTRL_XN) >> MMU_CTRL_XN_OFFSET) << 3);

    mtx_unlock(&dynmem_region_lock);
out:
    return retval;
}
