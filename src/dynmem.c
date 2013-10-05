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

/**
 * Dynmemmap
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

/**
 * Internal for this module struct that is updated by dynmem_get_region and can
 * be used when calling functions that use mmu_region_t structs.
 */
static mmu_pagetable_t dynmem_pt = {
    .vaddr          = 0x0,
    .pt_addr        = 0x0,
    .master_pt_addr = MMU_PT_BASE,
    .type           = MMU_PTT_COARSE,
    .dom            = MMU_DOM_USER
};

static mmu_region_t dynmem_region;

static void clear_dynmem_page_tables(void);
static int update_dynmem_region(void * p);
static uint32_t vaddr_to_pt_addr(uint32_t vaddr);


/**
 * Allocate and map a contiguous memory region from dynmem area.
 * @param refdef allocation definition.
 * @return address to the allocated region. Returns 0 if out of memory.
 */
void * dynmem_alloc_region(mmu_dregion_def refdef)
{
    /*TODO Find a continuous block of memory and allocate it by marking
     * it used in dynmemmap and then call mmu functions to map it correctly. */

    return 0;
}

/**
 * Free a memory region allocated by dynmem interface.
 * @param address address of the region to be freed.
 */
void dynmem_free_region(void * address)
{
    /* TODO Decrement ref count and if ref count = 0 => free the region by marking
     * dynmemmap locations free and calling region free function, which marks
     * region as fault. */

    update_dynmem_region(address);
    mmu_unmap_region(&dynmem_region);
}

/**
 * Update kernel dynamic page table entries.
 * Fills dynmem page tables with 1:1 mapping.
 */
void dynmem_update_kdpt()
{
    uint32_t i;

    clear_dynmem_page_tables();

    i = 0;
    do {
        if ((dynmemmap[i] & DYNMEM_RC_MASK) > 0) {
            update_dynmem_region((void *)(DYNMEM_START + i * 4096));
            /* Gives 1:1 mapping */
            if (!mmu_map_region(&dynmem_region)) {
                i += dynmem_region.num_pages;
            } else { /* error */
                i++;
            }
        } else { /* page unused */
            i++;
        }
    } while (i < DYNMEM_MAPSIZE);
}

/**
 * Fill dynmem page tables with FAULT entries.
 */
static void clear_dynmem_page_tables(void)
{
    uint32_t * pt = (uint32_t *)MMU_PT_FIRST_DYNPT;
    uint32_t i;

    for (i = 0; i < MMU_DYNMEM_PT_COUNT; i++) {
        pt[i] = MMU_PTE_FAULT;
    }
}

/**
 * Get a region structure and page table of a dynmem memory region.
 * Updates dynmem_pt and dynmem_region structures.
 * @note Region may map over several page tables but that's ok as dynmem
 * page tables are stored over contiguous memory space.
 * @param p pointer to the begining of the allocated dynmem section.
 * @return Error code.
 */
static int update_dynmem_region(void * p)
{
    uint32_t i;
    uint32_t reg_start = (uint32_t)p - DYNMEM_START;
    uint32_t reg_end;

    if ((dynmemmap[reg_start] & DYNMEM_RC_MASK) == 0) {
        KERROR(KERROR_ERR, "Unable to update dynmem_region.");
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

    dynmem_region.vaddr = DYNMEM_START + reg_start; /* 1:1 mapping by default */
    dynmem_region.num_pages = reg_end - reg_start;
    dynmem_region.ap = dynmemmap[reg_start] >> DYNMEM_AP_POS;
    dynmem_region.control = dynmemmap[reg_start] >> DYNMEM_CTRL_POS;
    dynmem_region.paddr = DYNMEM_START + reg_start; /* dynmem spec */

    /* Update single-use page table struct */
    dynmem_pt.vaddr = dynmem_region.vaddr;
    dynmem_pt.pt_addr = vaddr_to_pt_addr(dynmem_region.vaddr);
    dynmem_region.pt = &dynmem_pt;

    return 0;
}

/**
 * Convert a virtual dynmem address to a page table address in pt region.
 * @param vaddr any virtual address.
 * @return Returns a page table address in dynmem page table region.
 */
static uint32_t vaddr_to_pt_addr(uint32_t vaddr)
{
    uint32_t index, pt_addr;

    /* 1MB index stepping */
    index = (vaddr - DYNMEM_START) >> 20;
    pt_addr = MMU_PT_FIRST_DYNPT + index * DYNMEM_PT_SIZE;

    return pt_addr;
}
