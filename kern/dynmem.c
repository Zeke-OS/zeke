/**
 *******************************************************************************
 * @file    dynmem.c
 * @author  Olli Vanhoja
 * @brief   Dynmem management.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <kstring.h>
#include <kerror.h>
#include <klocks.h>
#include <sys/sysctl.h>
#include <bitmap.h>
#include <dynmem.h>

#define DYNMEM_RC_POS       16
#define DYNMEM_RL_POS       13
#define DYNMEM_AP_POS       10
#define DYNMEM_CTRL_POS     0

/** Ref count mask */
#define DYNMEM_RC_MASK      (0xFFFF0000)
/** Region Link mask */
#define DYNMEM_RL_MASK      (0x003 << DYNMEM_RL_POS)
/** Region control mask */
#define DYNMEM_CTRL_MASK    (0x3FF << DYNMEM_CTRL_POS)
/** Region access permissions mask */
#define DYNMEM_AP_MASK      (0x007 << DYNMEM_AP_POS)

/* Region Link bits */
/** No Link */
#define DYNMEM_RL_NL        (0x0 << DYNMEM_RL_POS)
/** Begin Link/Continue Link */
#define DYNMEM_RL_BL        (0x1 << DYNMEM_RL_POS)
/** End Link */
#define DYNMEM_RL_EL        (0x2 << DYNMEM_RL_POS)

/* TODO Is there any reason to store AP & Control here? */

#define FLAGS_TO_MAP(AP, CTRL)  ((AP << DYNMEM_AP_POS) | CTRL)
#define MAP_TO_AP(VAL)          ((VAL & DYNMEM_AP_MASK) >> DYNMEM_AP_POS)
#define MAP_TO_CTRL(VAL)        ((VAL & DYNMEM_CTRL_MASK) >> DYNMEM_CTRL_POS)

#if E2BITMAP_SIZE(DYNMEM_MAPSIZE) > 0
#define DYNMEM_BITMAPSIZE       E2BITMAP_SIZE(DYNMEM_MAPSIZE)
#else
#define DYNMEM_BITMAPSIZE       1
#endif

/**
 * Dynmemmap allocation table.
 *
 * dynmemmap format
 * ----------------
 *
 * |31       16| 15|14 13|12  10|9       0|
 * +-----------+---+-----+------+---------+
 * | ref count | X | RL  |  AP  | Control |
 * +-----------+---+-----+------+---------+
 *
 * RL = Region link
 * AP = Access Permissions
 * X  = Don't care
 */
uint32_t dynmemmap[DYNMEM_MAPSIZE];
uint32_t dynmemmap_bitmap[DYNMEM_BITMAPSIZE];

/**
 * Struct for temporary storage.
 */
static mmu_region_t dynmem_region;

/**
 * Lock used to protect dynmem_region struct, dynmem_bitmap and dynmemmap
 * access.
 */
static mtx_t dynmem_region_lock;

/* Some sysctl stat variables.
 */
static size_t dynmem_free = DYNMEM_END - DYNMEM_START + 1;
SYSCTL_UINT(_vm, OID_AUTO, dynmem_free, CTLFLAG_RD, (void *)(&dynmem_free), 0,
        "Amount of free dynmem");

static const size_t dynmem_tot = DYNMEM_END - DYNMEM_START + 1;
SYSCTL_UINT(_vm, OID_AUTO, dynmem_tot, CTLFLAG_RD, (void *)(&dynmem_tot), 0,
        "Total amount of dynmem");

static void * kmap_allocation(size_t pos, size_t size, uint32_t ap, uint32_t control);
static int update_dynmem_region_struct(void * p);
static int validate_addr(const void * addr, int test);

HW_PREINIT_ENTRY(dynmem_init);
void dynmem_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(mmu_init);

    mtx_init(&dynmem_region_lock, MTX_DEF | MTX_SPIN);

    SUBSYS_INITFINI("dynmem OK");
}

void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t control)
{
    uint32_t pos;
    void * retval = 0;

    mtx_spinlock(&dynmem_region_lock);

    if(bitmap_block_search(&pos, size, dynmemmap_bitmap,
                sizeof(dynmemmap_bitmap))) {
#if configDEBUG >= KERROR_DEBUG
        char buf[80];
        ksprintf(buf, sizeof(buf), "Out of dynmem, free %u/%u, tried to allocate %u MB",
                dynmem_free, dynmem_tot, size);
        KERROR(KERROR_DEBUG, buf);
#endif
        goto out;
    }

    /* Update sysctl stats */
    dynmem_free -= size * DYNMEM_PAGE_SIZE;

    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    retval = kmap_allocation((size_t)pos, size, ap, control);

out:
    mtx_unlock(&dynmem_region_lock);
    return retval;
}

void * dynmem_alloc_force(void * addr, size_t size, uint32_t ap, uint32_t control)
{
    size_t pos = (size_t)addr - DYNMEM_START;
    void * retval;

    if (!validate_addr(addr, 0)) {
        char buf[80];
        ksprintf(buf, sizeof(buf), "Invalid address; dynmem_alloc_force(): %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        return 0;
    }

    mtx_spinlock(&dynmem_region_lock);
    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    retval = kmap_allocation(pos, size, ap, control);
    mtx_unlock(&dynmem_region_lock);

    return retval;
}

void * dynmem_ref(void * addr)
{
    size_t i = (size_t)addr - DYNMEM_START;

    if (!validate_addr(addr, 1)) {
        char buf[80];
        ksprintf(buf, sizeof(buf), "Invalid address: %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        return 0;
    }
    if ((dynmemmap[i] & DYNMEM_RC_MASK) == 0)
        return 0;

    mtx_spinlock(&dynmem_region_lock);
    dynmemmap[i] += 1 << DYNMEM_RC_POS;
    mtx_unlock(&dynmem_region_lock);
    return addr;
}

void dynmem_free_region(void * addr)
{
    uint32_t i, j, rc;

    if (!validate_addr(addr, 1)) {
        char buf[40];
        ksprintf(buf, sizeof(buf), "Invalid address: %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        return;
    }

    i = (uint32_t)addr - DYNMEM_START;
    rc = (dynmemmap[i] & DYNMEM_RC_MASK) >> DYNMEM_RC_POS;

    /* Get lock to dynmem_region */
    mtx_spinlock(&dynmem_region_lock);

    /* Check if there is any references */
    if (rc > 1) {
        dynmemmap[i] -= 1 << DYNMEM_RC_POS;
        goto out; /* Do not free yet. */
    }

    if (update_dynmem_region_struct(addr)) { /* error */
        char buf[80];
        ksprintf(buf, sizeof(buf), "Can't free dynmem region: %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        goto out;
    }

    mmu_unmap_region(&dynmem_region);

    /* Mark the region as unused. */
    for (j = i; j < dynmem_region.num_pages; j++) {
        dynmemmap[j] = 0;
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
 * @param control   Control bits.
 */
static void * kmap_allocation(size_t base, size_t size, uint32_t ap, uint32_t control)
{
    size_t i;
    uint32_t mapflags = FLAGS_TO_MAP(ap, control);
    uint32_t rlb = (size > 1) ? DYNMEM_RL_BL : DYNMEM_RL_NL;
    uint32_t rle = (size > 1) ? DYNMEM_RL_EL : DYNMEM_RL_NL;
    uint32_t rc = (1 << DYNMEM_RC_POS);
    uint32_t addr = DYNMEM_START + base * 1048576;

    for (i = base; i < base + size - 1; i++) {
        dynmemmap[i] = rc | rlb | mapflags;
    }
    dynmemmap[i] = rc | rle | mapflags;

    /* Map memory region to the kernel memory space */
    dynmem_region.vaddr = addr;
    dynmem_region.paddr = addr;
    dynmem_region.num_pages = size;
    dynmem_region.ap = ap;
    dynmem_region.control = control;
    dynmem_region.pt = &mmu_pagetable_master;
    mmu_map_region(&dynmem_region);

    return (void *)addr;
}

/**
 * Update the region strucure to describe a already allocated dynmem region.
 * Updates dynmem_region structures.
 * @note dynmem_region_lock must be held before calling this function.
 * @param p Pointer to the begining of the allocated dynmem section (physical).
 * @return  Error code.
 */
static int update_dynmem_region_struct(void * base)
{
    uint32_t i, flags;
    uint32_t reg_start = (uint32_t)base - DYNMEM_START;
    uint32_t reg_end;
#if configDEBUG >= KERROR_ERR
    char buf[80];

    if (!validate_addr(base, 1)) {
        ksprintf(buf, sizeof(buf), "Invalid dynmem region addr: %x",
                (uint32_t)base);
        KERROR(KERROR_ERR, buf);
        return -1;
    }
#endif

    i = reg_start;
    if (dynmemmap[i] & DYNMEM_RL_BL) {
        /* Region linking begins */
        while (++i <= DYNMEM_END) {
            if (!(dynmemmap[i] & DYNMEM_RL_EL)) {
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
    dynmem_region.ap = MAP_TO_AP(flags);
    dynmem_region.control = MAP_TO_CTRL(flags);
    dynmem_region.pt = &mmu_pagetable_master;

    return 0;
}

void * dynmem_clone(void * addr)
{
    mmu_region_t cln;
    void * new_region;

    /* Take a reference to protect clone operation from concurrent
     * dynmem_free_region() call. */
    if (dynmem_ref(addr)) {
#if configDEBUG >= KERROR_ERR
        char buf[80];
        ksprintf(buf, sizeof(buf), "Can't clone given dynmem area @ %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
#endif
        return 0;
    }

    /* Take a copy of dynmem_region struct for this region. */
    mtx_spinlock(&dynmem_region_lock);
    if (update_dynmem_region_struct(addr)) { /* error */
#if configDEBUG  >= KERROR_ERR
        char buf[80];
        ksprintf(buf, sizeof(buf), "Clone failed @ %x", (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
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
        char buf[80];
        ksprintf(buf, sizeof(buf), "Out of dynmem while cloning @ %x",
                (uint32_t)addr);
#endif
        return 0;
    }

    /* NOTE: We don't need lock here as dynmem_ref quarantees that the region is
     * not removed during copy operation. */
    memcpy(new_region, (void *)(cln.paddr), cln.num_pages * 1048576);

    dynmem_free_region(addr);

    return new_region;
}

uint32_t dynmem_acc(const void * addr, size_t len)
{
    size_t size;
    uint32_t retval = 0;

    if (!validate_addr(addr, 1))
        goto out; /* Address out of bounds. */

    mtx_spinlock(&dynmem_region_lock);
    if (update_dynmem_region_struct((void *)addr)) { /* error */
#if configDEBUG >= KERROR_ERR
        char buf[80];
        ksprintf(buf, sizeof(buf), "dynmem_acc() check failed for: %x",
            (uint32_t)addr);
        KERROR(KERROR_DEBUG, buf);
#endif
        goto out;
    }

    /* Get size */
    if (!(size = mmu_sizeof_region(&dynmem_region))) {
#if configDEBUG >= KERROR_WARN
        char buf[80];
        ksprintf(buf, sizeof(buf), "Possible dynmem corruption at: %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
#endif
        goto out; /* Error in size calculation. */
    }
    if ((size_t)addr < dynmem_region.paddr
            || (size_t)addr > (dynmem_region.paddr + size)) {
        goto out; /* Not in region range. */
    }

    /* Acc seems to be ok.
     * Calc ap + xn as a return value for further testing.
     */
    retval = dynmem_region.ap |
        (((dynmem_region.control & MMU_CTRL_XN) >> MMU_CTRL_XN_OFFSET) << 3);
    mtx_unlock(&dynmem_region_lock);
out:
    return retval;
}

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

    if ((size_t)addr < DYNMEM_START || (size_t)addr > DYNMEM_END)
        return 0; /* Not in range */

    if (test && (((dynmemmap[i] & DYNMEM_RC_MASK) >> DYNMEM_RC_POS) == 0))
        return 0; /* Not allocated */

    return (1 == 1);
}
