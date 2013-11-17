/**
 *******************************************************************************
 * @file    ptmapper.c
 * @author  Olli Vanhoja
 * @brief   Page table mapper.
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
#include <generic/bitmap.h>
#include <ptmapper.h>

void ptmapper_init(void);

/* Fixed Page Tables */

/* Kernel master page table (L1) */
mmu_pagetable_t mmu_pagetable_master = {
    .vaddr          = 0,
    .pt_addr        = 0, /* These will be set */
    .master_pt_addr = 0, /* later in the init phase */
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_KERNEL
};

mmu_pagetable_t mmu_pagetable_system = {
    .vaddr          = MMU_VADDR_KERNEL_START,
    .pt_addr        = 0,
    .master_pt_addr = 0,
    .type           = MMU_PTT_COARSE,
    .dom            = MMU_DOM_KERNEL
};

/* Fixed Regions */

/* TODO Temporarily mapped as one big area */
mmu_region_t mmu_region_kernel = {
    .vaddr          = MMU_VADDR_KERNEL_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(MMU_VADDR_KERNEL_START, MMU_VADDR_KERNEL_END, 4096),
    .ap             = MMU_AP_RWRW, /* Todo this must be changed later to RWNA */
    .control        = MMU_CTRL_MEMTYPE_WB,
    .paddr          = 0x0,
    .pt             = &mmu_pagetable_system
};

mmu_region_t mmu_region_shared = {
    .vaddr          = MMU_VADDR_SHARED_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(MMU_VADDR_SHARED_START, MMU_VADDR_SHARED_END, 4096),
    .ap             = MMU_AP_RWRO,
    .control        = MMU_CTRL_MEMTYPE_WT,
    .paddr          = MMU_VADDR_SHARED_START,
    .pt             = &mmu_pagetable_system
};

#define PTREGION_SIZE 2 /* MB */
mmu_region_t mmu_region_page_tables = {
    .vaddr          = PTMAPPER_BASE,
    .num_pages      = PTREGION_SIZE,
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WT | MMU_CTRL_XN,
    .paddr          = PTMAPPER_BASE,
    .pt             = &mmu_pagetable_master
};

/**
 * Coarse page tables per MB.
 * Number of page tables that can be stored in one MB.
 * @note MMU_PTSZ_MASTER is a multiple of MMU_PTSZ_COARSE.
 */
#define PTS_PER_MB ((1024 * 1024) / MMU_PTSZ_COARSE)

/**
 * Page table region allocation bitmap.
 */
uint32_t ptm_alloc_map[E2BITMAP_SIZE(PTREGION_SIZE * PTS_PER_MB)];
#undef PTS_PER_MB

#define PTM_SIZEOF_MAP sizeof(ptm_alloc_map)
#define PTM_MASTER  0x10
#define PTM_COARSE  0x01

/**
 * Allocate a free block in ptm_alloc_map.
 * @param retval[out]   Index of the first contiguous block of requested length.
 * @param len           Block size either PTM_MASTER or PTM_COARSE.
 * @return Returns zero if a free block found; Value other than zero if there
 *         is no free contiguous block of requested length.
 */
#define PTM_ALLOC(retval, len) bitmap_block_alloc(retval, len, \
        ptm_alloc_map, PTM_SIZEOF_MAP)

#define PTM_BLOCK2ADDR(block) (PTMAPPER_BASE + block * MMU_PTSZ_COARSE)

extern void halt();

/**
 * Page table mapper init function.
 * @note This function should be called by mmu init.
 */
void ptmapper_init(void)
{
    /* Allocate memory for mmu_pagetable_master */
    if (ptmapper_alloc(&mmu_pagetable_master)) {
        KERROR(KERROR_ERR, "Can't allocate memory for master page table.");
        while(1); /* TODO Replace with something else */
    }

    mmu_pagetable_system.master_pt_addr = mmu_pagetable_master.master_pt_addr;
    if (ptmapper_alloc(&mmu_pagetable_system)) {
        KERROR(KERROR_ERR, "Can't allocate memory for system page table.");
        while(1);
    }

    /* Initialize system page tables */
    mmu_init_pagetable(&mmu_pagetable_master);
    mmu_init_pagetable(&mmu_pagetable_system);

    /* Fill page tables with translations & attributes */
    mmu_map_region(&mmu_region_kernel);
    mmu_map_region(&mmu_region_shared);
    mmu_map_region(&mmu_region_page_tables);

    /* Activate page tables */
    mmu_attach_pagetable(&mmu_pagetable_master); /* Load L1 TTB */
    mmu_attach_pagetable(&mmu_pagetable_system); /* Add L2 pte into L1 master pt */
}

/**
 * Allocate memory for a page table.
 * @note master_pt_addr will be only set for a page table struct if allocated
 * page table is a master page table.
 * @param pt    is the page table struct without page table address pt_addr.
 * @return  Zero if succeed; Otherwise value other than zero indicating that
 *          the page table allocation has failed.
 */
int ptmapper_alloc(mmu_pagetable_t * pt)
{
    size_t block;
    size_t addr;
    size_t size = 0;
    int retval = 0;

    switch (pt->type) {
        case MMU_PTT_MASTER:
            size = PTM_MASTER;
            break;
        case MMU_PTT_COARSE:
            size = PTM_COARSE;
            break;
        default:
            break;
    }

    /* Allocate memory for mmu_pagetable_master */
    if ((size == 0) ? 0 : !PTM_ALLOC(&block, PTM_MASTER)) {
        addr = PTM_BLOCK2ADDR(block);
        pt->pt_addr = addr;
        if (pt->type == MMU_PTT_MASTER) {
            pt->master_pt_addr = addr;
        }
    } else {
        retval = 1;
    }

    return retval;
}
