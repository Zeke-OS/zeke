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

#include <kstring.h>
#include <kerror.h>
#include <generic/bitmap.h>
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

/**
 * Dynmemmap allocation table.
 *
 * dynmemmap format
 * ----------------
 *
 * |31       16|15|14 13|12 10|9       0|
 * +-----------+--+-----+-----+---------+
 * | ref count |X | RL  |  AP | Control |
 * +-----------+--+-----+-----+---------+
 *
 * RL = Region link
 * AP = Access Permissions
 * X  = Don't care
 */
uint32_t dynmemmap[DYNMEM_MAPSIZE];
uint32_t dynmemmap_bitmap[E2BITMAP_SIZE(DYNMEM_MAPSIZE)];

/**
 * Struct for temporary storage.
 * @todo TODO Add locks for MP/preempt.
 */
static mmu_region_t dynmem_region;

static void * kmap_allocation(size_t pos, size_t size, uint32_t ap, uint32_t control);
static int update_dynmem_region_struct(void * p);

/*
 * TODO Add variables for sysctl
 */

/**
 * Allocate a contiguous memory region from dynmem area.
 * @param size      Region size in 1MB blocks.
 * @param ap        Access permission.
 * @param control   Control settings.
 * @return  Address to the allocated region. Returns 0 if out of memory.
 */
void * dynmem_alloc_region(size_t size, uint32_t ap, uint32_t control)
{
    uint32_t pos;

    if(bitmap_block_search(&pos, size, dynmemmap_bitmap, sizeof(dynmemmap_bitmap))) {
        KERROR(KERROR_ERR, "Out of dynmem.");
        return 0; /* Null */
    }

    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    return kmap_allocation((size_t)pos, size, ap, control);
}

/**
 * Forces a new memory region allocation from the given address even if it's
 * already reserved.
 * This function will never fail, and might be destructive and may even
 * corrupt the allocation table.
 * @param addr  Address.
 * @param size  Region size in 1MB blocks.
 * @param ap    Access permission.
 * @param control Control settings.
 * @return  address to the allocated region.
 */
void * dynmem_alloc_force(void * addr, size_t size, uint32_t ap, uint32_t control)
{
    size_t pos = (size_t)addr - DYNMEM_START;

    bitmap_block_update(dynmemmap_bitmap, 1, pos, size);
    return kmap_allocation(pos, size, ap, control);
}

/**
 * Decrement dynmem region reference counter. If the final value of a reference
 * counter is zero then the dynmem region is freed and unmapped.
 * @param addr  Address of the dynmem region to be freed.
 */
void dynmem_free_region(void * addr)
{
    uint32_t i, j, rc;
    char buf[40];

    i = (uint32_t)addr - DYNMEM_START;
    rc = (dynmemmap[i] & DYNMEM_RC_MASK) >> DYNMEM_RC_POS;

    if (--rc > 0) {
        dynmemmap[i] = rc << DYNMEM_RC_POS;
        return;
    }

    if (update_dynmem_region_struct(addr)) { /* error */
        ksprintf(buf, sizeof(buf), "Can't free dynmem region: %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        return;
    }

    mmu_unmap_region(&dynmem_region);

    /* Mark the region as unused. */
    for (j = i; j < dynmem_region.num_pages; j++) {
        dynmemmap[j] = 0;
    }
    bitmap_block_update(dynmemmap_bitmap, 1, j, dynmem_region.num_pages);
}

/**
 * Updates dynmem allocation table and intially maps the memory region to the
 * kernel memory space.
 * @param base      is the base address.
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
    uint32_t addr = DYNMEM_START + base;

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
 * @param p Pointer to the begining of the allocated dynmem section.
 * @return  Error code.
 */
static int update_dynmem_region_struct(void * base)
{
    uint32_t i, flags;
    uint32_t reg_start = (uint32_t)base - DYNMEM_START;
    uint32_t reg_end;
    char buf[80];

    if ((uint32_t)base < DYNMEM_START) {
        ksprintf(buf, sizeof(buf), "Invalid dynmem region addr: %x", (uint32_t)base);
        KERROR(KERROR_ERR, buf);
        return -1;
    }

    /* If ref count == 0 then there is no allocation. */
    if ((dynmemmap[reg_start] & DYNMEM_RC_MASK) == 0) {
        KERROR(KERROR_ERR, "Invalid dynmem region addr or not alloc.");
        return -2;
    }

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

/**
 * Test for dynmem access.
 * Return value format:
 * 3  2    0
 * +--+----+
 * |XN| AP |
 * +--+----+
 * @param addr  is the base address.
 * @param len   is the size of block tested.
 * @return Returns 0 if addr is invalid; Otherwise returns ap flags + xn bit.
 */
uint32_t dynmem_acc(void * addr, size_t len)
{
    size_t i;
    size_t size;
    uint32_t retval = 0;

    if (addr < DYNMEM_START || addr > DYNMEM_END)
        goto out; /* Address out of bounds. */

    i = (size_t)addr - DYNMEM_START;

    if (((dynmemmap[i] & DYNMEM_RC_MASK) >> DYNMEM_RC_POS) == 0)
        goto out; /* Not reserved. */

    if (update_dynmem_region_struct(addr)) { /* error */
        char buf[80];
        ksprintf(buf, sizeof(buf), "dynmem_acc() check failed for: %x",
            (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        goto out;
    }

    /* Get size */
    if(!(size = mmu_sizeof_region(&dynmem_region))) {
        char buf[80];ksprintf(buf, sizeof(buf), "Possible dynmem corruption at: %x",
                (uint32_t)addr);
        KERROR(KERROR_ERR, buf);
        goto out; /* Error in size calculation. */
    }
    if (addr < dynmem_region.vaddr || addr > (dynmem_region.vaddr + size))
        goto out; /* Not in region range. */

    /* Acc seems to be ok.
     * Calc ap + xn as a return value for further testing.
     */
    retval = dynmem_region.ap |
        (((dynmem_region.control & MMU_CTRL_XN) >> MMU_CTRL_XN_OFFSET) << 3);
out:
    return retval;
}
