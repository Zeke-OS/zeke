/**
 *******************************************************************************
 * @file    kmem.c
 * @author  Olli Vanhoja
 * @brief   Kernel static memory mappings.
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

#include <kerror.h>
#include <kmem.h>
#include <kstring.h>
#include <ptmapper.h>

/* Fixed Page Tables **********************************************************/

/* Kernel master page table (L1) */
mmu_pagetable_t mmu_pagetable_master = {
    .vaddr          = 0,
    .pt_addr        = 0, /* These will be set       */
    .nr_tables      = 1,
    .master_pt_addr = 0, /* later in the init phase */
    .pt_type        = MMU_PTT_MASTER,
    .pt_dom         = MMU_DOM_KERNEL
};

struct vm_pt vm_pagetable_system = {
    .pt = {
        .vaddr          = 0, /* Start */
        .pt_addr        = 0, /* Will be set later */
        .nr_tables      = 0, /* Will be set later */
        .master_pt_addr = 0, /* Will be set later */
        .pt_type        = MMU_PTT_COARSE,
        .pt_dom         = MMU_DOM_KERNEL
    },
};

/* Kernel Fixed Regions *******************************************************/

/** Kernel mode stacks, other than thread kernel stack. */
const mmu_region_t mmu_region_kstack = {
    .vaddr          = configKSTACK_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(
                        configKSTACK_START, configKSTACK_END,
                        MMU_PGSIZE_COARSE),
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WB | MMU_CTRL_XN,
    .paddr          = configKSTACK_START,
    .pt             = &vm_pagetable_system.pt
};

extern void *  _rodata_end __attribute__((weak));
/** Read-only kernel code &  ro-data */
mmu_region_t mmu_region_kernel = {
    .vaddr          = configKERNEL_START,
    .num_pages      = 0, /* Set in init */
    .ap             = MMU_AP_RONA,
    .control        = MMU_CTRL_MEMTYPE_WB,
    .paddr          = configKERNEL_START,
    .pt             = &vm_pagetable_system.pt
};
KMEM_FIXED_REGION(mmu_region_kernel);

/** Start of the kernel rw region. */
extern void * _data_start __attribute__((weak));
/** Last static variable in bss. */
extern void * __bss_break __attribute__((weak));
/** End of kernel memory region by linker.
 * This variable is not actually very useful for anything as we'll anyway
 * end at full mega byte. */
extern void * _end __attribute__((weak));
mmu_region_t mmu_region_kdata = {
    .vaddr          = 0, /* Set in init */
    .num_pages      = 0, /* Set in init */
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WB | MMU_CTRL_XN,
    .paddr          = 0, /* Set in init */
    .pt             = &vm_pagetable_system.pt
};
KMEM_FIXED_REGION(mmu_region_kdata);

int _kmem_ready; /* Atomic operations can be safely used after this is set. */

/**
 * Initialize the kernel memory map.
 * This function is called from kinit.c
 */
void kmem_init(void)
{
    /* Allocate memory for mmu_pagetable_master */
    if (ptmapper_alloc(&mmu_pagetable_master)) {
        /* Critical failure */
        panic("Can't allocate memory for master page table.\n");
    }

    vm_pagetable_system.pt.master_pt_addr = mmu_pagetable_master.master_pt_addr;
    vm_pagetable_system.pt.nr_tables =
        (configKERNEL_END + 1) / MMU_PGSIZE_SECTION;
    if (ptmapper_alloc(&vm_pagetable_system.pt)) {
        /* Critical failure */
        panic("Can't allocate memory for system page table.\n");
    }

    /* Initialize system page tables */
    mmu_init_pagetable(&mmu_pagetable_master);
    mmu_init_pagetable(&vm_pagetable_system.pt);

    /*
     * Init regions
     */

    /* Kernel ro region */
    mmu_region_kernel.num_pages  = MMU_PAGE_CNT_BY_RANGE(
            configKERNEL_START, (intptr_t)(&_rodata_end) - 1,
            MMU_PGSIZE_COARSE);
    /* Kernel rw data region */
    mmu_region_kdata.vaddr      = (intptr_t)(&_data_start);
    mmu_region_kdata.num_pages  = MMU_PAGE_CNT_BY_RANGE(
            (intptr_t)(&_data_start), configKERNEL_END,
            MMU_PGSIZE_COARSE);
    mmu_region_kdata.paddr      = (intptr_t)(&_data_start);

    /* Fill page tables with translations & attributes */
    {
        mmu_region_t ** regp;
#if defined(configKMEM_DEBUG)
        const char str_type[2][9] = {"sections", "pages"};
#define PRINTMAPREG(region)                             \
        KERROR(KERROR_DEBUG, "Mapped %s: %u %s\n",      \
            #region, region.num_pages,                  \
            (region.pt->pt_type == MMU_PTT_MASTER) ?    \
                str_type[0] : str_type[1]);
#else
#define PRINTMAPREG(region)
#endif
#define MAP_REGION(reg)         \
        mmu_map_region(&reg);   \
        PRINTMAPREG(reg)
        MAP_REGION(mmu_region_kstack);
#undef MAP_REGION
#undef PRINTMAPREG

        SET_FOREACH(regp, kmem_fixed_regions) {
            mmu_map_region(*regp);
        }
    }

    /* Activate page tables */
    mmu_attach_pagetable(&vm_pagetable_system.pt); /* Add L2 pte into L1 mpt */
    mmu_attach_pagetable(&mmu_pagetable_master); /* Load L1 TTB */

    _kmem_ready = 1;
}
