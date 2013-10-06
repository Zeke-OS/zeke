/**
 *******************************************************************************
 * @file    dynmem.c
 * @author  Olli Vanhoja
 * @brief   Dynmem management.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kerror.h>
#include <dynmem.h>

#define DYNMEM_RC_POS       0
#define DYNMEM_RL_POS       18
#define DYNMEM_CTRL_POS     20
#define DYNMEM_AP_POS       30

/** Ref Count mask */
#define DYNMEM_RC_MASK      0x3FFFF
/** Region Link mask */
#define DYNMEM_RL_MASK      (0x003 << DYNMEM_RL_POS)
/** Region control mask */
#define DYNMEM_CTRL_MASK    (0x3FF << DYNMEM_CTRL_POS)
/** Region access permissions mask */
#define DYNMEM_AP_MASK      (0x002 << DYNMEM_AP_POS)

/* Region Link bits */
/** No Link */
#define DYNMEM_RL_NL        (0x0 << DYNMEM_RL_POS)
/** Begin Link/Continue Link */
#define DYNMEM_RL_BL        (0x1 << DYNMEM_RL_POS)
/** End Link */
#define DYNMEM_RL_EL        (0x2 << DYNMEM_RL_POS)

/* TODO Is there any reason to store AP & Control here? */

/**
 * Dynmemmap allocation table.
 *
 * dynmemmap format
 * ----------------
 *
 * |31  30|29     20|19 18|17        0|
 * +------+---------+-----+-----------+
 * |  AP  | Control | RL  | ref count |
 * +------+---------+-----+-----------+
 *
 * RL = Region link
 * AP = Access Permissions
 */
uint32_t dynmemmap[DYNMEM_MAPSIZE];
uint32_t dynmemmap_bitmap[DYNMEM_MAPSIZE / 32];

static mmu_region_t dynmem_region;

static int bitmap_block_search(uint32_t * retval, uint32_t block_len, uint32_t * bitmap, uint32_t size);
static void bitmap_block_update(uint32_t * bitmap, uint32_t mark, uint32_t start, uint32_t len);
static int update_dynmem_region(void * p);
static uint32_t vaddr_to_pt_addr(uint32_t vaddr);


/**
 * Allocate and map a contiguous memory region from dynmem area.
 * @param size region size in 1MB blocks.
 * @param ap access permission.
 * @param control control settings.
 * @return address to the allocated region. Returns 0 if out of memory.
 */
void * dynmem_alloc_region(uint32_t size, uint32_t ap, uint32_t control)
{
    /* TODO update dynmemmap & update ap+control to the kernel master table & return pointer */
    uint32_t retval;
    retval = bitmap_block_search(&retval, size, dynmemmap_bitmap, sizeof(dynmemmap_bitmap));
    bitmap_block_update(dynmemmap_bitmap, 1, retval, size);

    return 0;
}

/**
 * Decrement dynmem region reference counter. If the final value of a reference
 * counter is zero then the dynmem region is freed and unmapped.
 * @param address address of the dynmem region to be freed.
 */
void dynmem_free_region(void * address)
{
    uint32_t i, j, rc;

    i = (uint32_t)address - DYNMEM_START;
    rc = dynmemmap[i] & DYNMEM_RC_MASK;

    if (rc > 1) {
        dynmemmap[i] = rc--;
        return;
    }

    if (update_dynmem_region(address)) { /* error */
        KERROR(KERROR_ERR, "Can't free dynmem region.");
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
 * Search for a contiguous block of block_len in bitmap.
 * @param retval[out] index of the first contiguous block of requested length.
 * @param block_len is the lenght of contiguous block searched for.
 * @param bitmap is a bitmap of block reservations.
 * @param size is the size of bitmap in bytes.
 * @return Returns zero if a free block found; Value other than zero if there is no free
 * contiguous block of requested length.
 */
static int bitmap_block_search(uint32_t * retval, uint32_t block_len, uint32_t * bitmap, uint32_t size)
{
    uint32_t i, j;
    uint32_t * cur;
    uint32_t start = 0, end = 0;

    block_len--;
    cur = &start;
    for (i = 0; i < (size / sizeof(uint32_t)); i++) {
        for(j = 0; j < 32; j++) {
            if ((bitmap[i] & (1 << j)) == 0) {
                *cur = i * 32 + j;
                cur = &end;

                if ((end - start >= block_len) && (end >= start)) {
                    *retval = start;
                    return 0;
                }
            } else {
                start = 0;
                end = 0;
                cur = &start;
            }
        }
    }

    return 1;
}

/**
 * Set or clear contiguous block of bits in bitmap.
 * @param bitmap is the bitmap being changed.
 * @param mark 0 = clear; 1 = set;
 * @param start is the starting bit position in bitmap.
 * @param len is the length of the block being updated.
 */
static void bitmap_block_update(uint32_t * bitmap, uint32_t mark, uint32_t start, uint32_t len)
{
    uint32_t i, j, n, tmp;
    uint32_t k = (start - (start & 31)) / 32;
    uint32_t max = k + len / 32 + 1;

    mark &= 1;

    /* start mod 32 */
    n = start & 31;

    j = 0;
    for (i = k; i <= max; i++) {
        while (n < 32) {
            tmp = mark << n;
            bitmap[i]  &= ~(1 << n);
            bitmap[i] |= tmp;
            n++;
            if (++j >= len)
                return;
        }
        n = 0;
    }
}

/**
 * Get a region structure of a dynmem memory region.
 * Updates dynmem_region structures.
 * @param p pointer to the begining of the allocated dynmem section.
 * @return Error code.
 */
static int update_dynmem_region(void * p)
{
    uint32_t i;
    uint32_t reg_start = (uint32_t)p - DYNMEM_START;
    uint32_t reg_end;

    if ((dynmemmap[reg_start] & DYNMEM_RC_MASK) == 0) {
        KERROR(KERROR_ERR, "Invalid dynmem region addr.");
        return -1;
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
    }
    reg_end = i;

    dynmem_region.vaddr = (uint32_t)p; /* 1:1 mapping by default */
    dynmem_region.paddr = (uint32_t)p;
    dynmem_region.num_pages = reg_end - reg_start + 1;
    dynmem_region.ap = dynmemmap[reg_start] >> DYNMEM_AP_POS;
    dynmem_region.control = dynmemmap[reg_start] >> DYNMEM_CTRL_POS;
    dynmem_region.pt = &mmu_pagetable_master;

    return 0;
}
