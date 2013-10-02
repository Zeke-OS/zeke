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

#include <dynmem.h>

#define DYNMEM_RC_POS       0
#define DYNMEM_RL_POS       19
#define DYNMEM_CTRL_POS     20
#define DYNMEM_AP_POS       30

#define DYNMEM_RC_MASK      0x7FFFF
#define DYNMEM_RL_MASK      (0x001 << DYNMEM_RL_POS)
#define DYNMEM_CTRL_MASK    (0x3FF << DYNMEM_CTRL_POS)
#define DYNMEM_AP_MASK      (0x002 << DYNMEM_AP_POS)

/**
 * Dynmemmap
 *
 * dynmemmap format
 * ----------------
 *
 * |31  30|29     20| 19 |18        0|
 * +------+---------+----+-----------+
 * |  AP  | Control | RL | ref count |
 * +------+---------+----+-----------+
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
    .master_pt_addr = MMU_PT_BASE, /* TODO fixme */
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_USER
};

static mmu_region_t dynmem_region;

/* TODO
 * - Init function that initializes dynmem page tables in pt region & maps them
 *   to master page table.
 */

static void dynmem_free_region_r();
static void update_dynmem_region(void * p);
static void update_dynmem_pt();
static uint32_t vaddr_to_pt_addr(uint32_t vaddr);


void * dynmem_alloc_region(mmu_dregion_def refdef)
{
    /*TODO Find a continuous block of memory and allocate it by marking
     * it used in dynmemmap and then call mmu functions to map it correctly. */

    return 0;
}

void dynmem_free_region(void * address)
{
    /* TODO Decrement ref count and if ref count = 0 => free the region by marking
     * dynmemmap locations free and calling region free function, which marks
     * region as fault. */

    update_dynmem_region(address);
    dynmem_free_region_r();
}

/**
 * Free region marked in dynmem_region
 */
static void dynmem_free_region_r()
{
    mmu_unmap_region(&dynmem_region);
}

/**
 * Get a region structure of a dynmem memory region.
 * TODO convert to module local region struct?
 * @param p pointer to the begining of the allocated dynmem section.
 */
static void update_dynmem_region(void * p)
{
    uint32_t i;
    uint32_t reg_start = (uint32_t)p;
    uint32_t reg_end;

    /* TODO check if region begins before p? */

    i = reg_start;
    if (dynmemmap[i] & DYNMEM_RL_MASK) {
        while (++i <= DYNMEM_END) {
            if (!(dynmemmap[i] & DYNMEM_RL_MASK)) {
                i--;
                break;
            }
        }
    }
    reg_end = i;

    dynmem_region.vaddr = DYNMEM_START + reg_start; /* At least this according to spec */
    dynmem_region.num_pages = reg_end - reg_start;
    dynmem_region.ap = dynmemmap[reg_start] >> DYNMEM_AP_POS;
    dynmem_region.control = dynmemmap[reg_start] >> DYNMEM_CTRL_POS;
    dynmem_region.paddr = DYNMEM_START + reg_start; /* dynmem spec */
    dynmem_region.pt = 0; /* TODO We don't have this entry for dynmem atm. */
}

static void update_dynmem_pt()
{
}

/**
 * Convert virtual dynmem address to a page table address.
 */
static uint32_t vaddr_to_pt_addr(uint32_t vaddr)
{
    /* TODO */
    /* MMU_PT_FIRST_DYNPT is the first dynamic page table */
}
