/**
 *******************************************************************************
 * @file    mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control.
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

/** @addtogroup HAL
  * @{
  */

#include <kinit.h>
#include <ptmapper.h>
#include "mmu.h"

void mmu_init(void);
SECT_HW_PREINIT(mmu_init);

/* Fixed Page Tables */

/* Kernel master page table (L1) */
mmu_pagetable_t mmu_pagetable_master = {
    .vaddr          = MMU_PT_BASE,
    .pt_addr        = MMU_PT_BASE,
    .master_pt_addr = MMU_PT_BASE,
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_KERNEL
};

mmu_pagetable_t mmu_pagetable_system = {
    .vaddr          = MMU_VADDR_KERNEL_START,
    .pt_addr        = MMU_PT_ADDR(0),
    .master_pt_addr = MMU_VADDR_MASTER_PT,
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

mmu_region_t mmu_region_page_tables = {
    .vaddr          = MMU_PT_BASE,
    .num_pages      = 2, /* TODO is 2 megs enough for everyone? */
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WT | MMU_CTRL_XN,
    .paddr          = MMU_PT_BASE,
    .pt             = &mmu_pagetable_master
};


/**
 * Initialize the MMU and static regions.
 * @note This is called from startup.
 */
void mmu_init(void)
{
    uint32_t value, mask;

    /* Initialize the fixed page tables */
    mmu_init_pagetable(&mmu_pagetable_master);
    mmu_init_pagetable(&mmu_pagetable_system);

    /* Fill page tables with translations & attributes */
    mmu_map_region(&mmu_region_kernel);
    mmu_map_region(&mmu_region_shared);
    mmu_map_region(&mmu_region_page_tables);

    /* Activate page tables */
    mmu_attach_pagetable(&mmu_pagetable_master); /* Load L1 TTB to cp15:c2:c0 */
    mmu_attach_pagetable(&mmu_pagetable_system); /* Load L2 pte into L1 PT */

    /* Set MMU_DOM_KERNEL as client and others to generate error. */
    value = MMU_DOMAC_TO(MMU_DOM_KERNEL, MMU_DOMAC_CL);
    mask = MMU_DOMAC_ALL;
    mmu_domain_access_set(value, mask);

    value = MMU_ZEKE_C1_DEFAULTS;
    mask = MMU_ZEKE_C1_DEFAULTS;
    mmu_control_set(value, mask);
}

/**
  * @}
  */
