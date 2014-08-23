/**
 *******************************************************************************
 * @file    ptmapper.c
 * @author  Olli Vanhoja
 * @brief   Page table mapper.
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
#include <bitmap.h>
#include <sys/sysctl.h>
#include <ptmapper.h>

void ptmapper_init(void);
HW_PREINIT_ENTRY(ptmapper_init);

/* Fixed Page Tables **********************************************************/

/* Kernel master page table (L1) */
mmu_pagetable_t mmu_pagetable_master = {
    .vaddr          = 0,
    .pt_addr        = 0, /* These will be set */
    .master_pt_addr = 0, /* later in the init phase */
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_KERNEL
};

mmu_pagetable_t mmu_pagetable_system = {
    .vaddr          = 0,
    .pt_addr        = 0, /* Set later */
    .master_pt_addr = 0, /* Set later */
    .type           = MMU_PTT_COARSE,
    .dom            = MMU_DOM_KERNEL
};

struct vm_pt vm_pagetable_system;

/* Fixed Regions **************************************************************/

/** Kernel mode stacks. */
const mmu_region_t mmu_region_kstack = {
    .vaddr          = MMU_VADDR_KSTACK_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(
                        MMU_VADDR_KSTACK_START, MMU_VADDR_KSTACK_END,
                        MMU_PGSIZE_COARSE),
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WB | MMU_CTRL_XN,
    .paddr          = MMU_VADDR_KSTACK_START,
    .pt             = &mmu_pagetable_system
};

#if 0
/* Kernel mode system/thread stack. */
mmu_region_t mmu_region_tkstack = {
    .vaddr          = MMU_VADDR_TKSTACK_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(
                        MMU_VADDR_TKSTACK_START, MMU_VADDR_TKSTACK_END, 4096),
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_XN,
    .paddr          = MMU_VADDR_TKSTACK_START, /* Temporarily 1:1 but this *
                                                * mapping is never used.   */
    .pt             = &mmu_pagetable_system
};
#endif

extern void *  _rodata_end __attribute__((weak));
/** Read-only kernel code &  ro-data */
mmu_region_t mmu_region_kernel = {
    .vaddr          = MMU_VADDR_KERNEL_START,
    .num_pages      = 0, /* Set in init */
    .ap             = MMU_AP_RORO, /* TODO this must be changed later to RONA */
    .control        = MMU_CTRL_MEMTYPE_SO,
    .paddr          = MMU_VADDR_KERNEL_START,
    .pt             = &mmu_pagetable_system
};

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
    .ap             = MMU_AP_RWRW, /* TODO */
    .control        = MMU_CTRL_MEMTYPE_WB,
    .paddr          = 0, /* Set in init */
    .pt             = &mmu_pagetable_system
};

#if 0
extern void (*__text_shared_start[]) (void) __attribute__((weak));
extern void (*__text_shared_end[]) (void) __attribute__((weak));
mmu_region_t mmu_region_shared = {
    .vaddr          = MMU_VADDR_SHARED_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(MMU_VADDR_SHARED_START, \
                        MMU_VADDR_SHARED_END, 4096),
    .ap             = MMU_AP_RWRO,
    .control        = MMU_CTRL_MEMTYPE_WT,
    .paddr          = 0, /* Will be set later. */
    .pt             = &mmu_pagetable_system
};
#endif

mmu_region_t mmu_region_rpihw = {
    .vaddr          = MMU_VADDR_RPIHW_START,
    .num_pages      = MMU_PAGE_CNT_BY_RANGE(MMU_VADDR_RPIHW_START, \
                        MMU_VADDR_RPIHW_END, MMU_PGSIZE_SECTION),
    .ap             = MMU_AP_RWRW, /* TODO */
    .control        = MMU_CTRL_MEMTYPE_SDEV | MMU_CTRL_XN,
    .paddr          = MMU_VADDR_RPIHW_START,
    .pt             = &mmu_pagetable_master
};

#define PTREGION_SIZE \
    MMU_PAGE_CNT_BY_RANGE(PTMAPPER_PT_START, PTMAPPER_PT_END, MMU_PGSIZE_SECTION)
mmu_region_t mmu_region_page_tables = {
    .vaddr          = PTMAPPER_PT_START,
    .num_pages      = PTREGION_SIZE,
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WT | MMU_CTRL_XN,
    .paddr          = PTMAPPER_PT_START,
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

static int ptm_nr_pt = 0;
SYSCTL_INT(_vm, OID_AUTO, ptm_nr_pt, CTLFLAG_RD, &ptm_nr_pt, 0,
    "Total number of page tables allocated.");

static size_t ptm_mem_free = PTREGION_SIZE * MMU_PGSIZE_SECTION;
SYSCTL_UINT(_vm, OID_AUTO, ptm_mem_free, CTLFLAG_RD, &ptm_mem_free, 0,
    "Amount of free page table region memory.");

static const size_t ptm_mem_tot = PTREGION_SIZE * MMU_PGSIZE_SECTION;
SYSCTL_UINT(_vm, OID_AUTO, ptm_mem_tot, CTLFLAG_RD,
    (unsigned int *)(&ptm_mem_tot), 0,
    "Total size of page table region.");

#define PTM_SIZEOF_MAP sizeof(ptm_alloc_map)
#define PTM_MASTER  0x10 /*!< Len of master page table in ptm_alloc_map. */
#define PTM_COARSE  0x01 /*!< Len of coarse page table in ptm_alloc_map. */

/**
 * Convert a block index to an address.
 */
#define PTM_BLOCK2ADDR(block) (PTMAPPER_PT_START + (block) * MMU_PTSZ_COARSE)

/**
 * Convert an address to a block index.
 */
#define PTM_ADDR2BLOCK(addr) (((addr) - PTMAPPER_PT_START) / MMU_PTSZ_COARSE)

/**
 * Allocate a free block in ptm_alloc_map.
 * @param retval[out]   Index of the first contiguous block of requested length.
 * @param len           Block size, either PTM_MASTER or PTM_COARSE.
 * @return Returns zero if a free block found; Value other than zero if there
 *         is no free contiguous block of requested length.
 */
#define PTM_ALLOC(retval, len, balign) \
    bitmap_block_align_alloc(retval, len, ptm_alloc_map, PTM_SIZEOF_MAP, \
            balign)

/**
 * Free a block that has been previously allocated.
 * @param block is the block that has been allocated with PTM_ALLOC.
 * @param len   is the length of the block, either PTM_MASTER or PTM_COARSE.
 */
#define PTM_FREE(block, len) \
    bitmap_block_update(ptm_alloc_map, 0, block, len)

/**
 * Page table mapper init function.
 * @note This function should be called by mmu init.
 */
void ptmapper_init(void)
{
    SUBSYS_INIT();
    KERROR(KERROR_INFO, "ptmapper init started");

    /* Allocate memory for mmu_pagetable_master */
    if (ptmapper_alloc(&mmu_pagetable_master)) {
        panic("Can't allocate memory for master page table.");
    }

    mmu_pagetable_system.master_pt_addr = mmu_pagetable_master.master_pt_addr;
    if (ptmapper_alloc(&mmu_pagetable_system)) {
        panic("Can't allocate memory for system page table.");
    }

    /* Initialize system page tables */
    mmu_init_pagetable(&mmu_pagetable_master);
    mmu_init_pagetable(&mmu_pagetable_system);

    /* Init regions */
    /* Kernel ro region */
    mmu_region_kernel.num_pages  = MMU_PAGE_CNT_BY_RANGE(
            MMU_VADDR_KERNEL_START, (intptr_t)(&_rodata_end) - 1,
            MMU_PGSIZE_COARSE);
    /* Kernel rw data region */
    mmu_region_kdata.vaddr      = (intptr_t)(&_data_start);
    mmu_region_kdata.num_pages  = MMU_PAGE_CNT_BY_RANGE(
            (intptr_t)(&_data_start), MMU_VADDR_KERNEL_END,
            MMU_PGSIZE_COARSE);
    mmu_region_kdata.paddr      = (intptr_t)(&_data_start);

    /* Fill page tables with translations & attributes */
    {
#if configDEBUG >= KERROR_DEBUG
        char buf[80];
        const char str_type[2][9] = {"sections", "pages"};
#define PRINTMAPREG(region) \
        ksprintf(buf, sizeof(buf), "Mapped %s: %u %s", \
            #region, region.num_pages, \
            (region.pt->type == MMU_PTT_MASTER) ? \
                str_type[0] : str_type[1]); \
        KERROR(KERROR_DEBUG, buf);
#else
#define PRINTMAPREG(region)
#endif
#define MAP_REGION(reg) \
        mmu_map_region(&reg); \
        PRINTMAPREG(reg)

        //MAP_REGION(mmu_region_tkstack);
        MAP_REGION(mmu_region_kstack);
        MAP_REGION(mmu_region_kernel);
        MAP_REGION(mmu_region_kdata);
        MAP_REGION(mmu_region_page_tables);
        MAP_REGION(mmu_region_rpihw);
#undef MAP_REGION
#undef PRINTMAPREG
    }

    /* Copy system page table to vm version of it, this is the only easy way to
     * solve some issues now. TODO Maybe we'd like to do some major refactoring
     * some day. */
    vm_pagetable_system.pt = mmu_pagetable_system;
    vm_pagetable_system.linkcount = 1;

    /* Activate page tables */
    mmu_attach_pagetable(&mmu_pagetable_master); /* Load L1 TTB */
#if configDEBUG >= KERROR_DEBUG
    KERROR(KERROR_DEBUG, "Attached TTB mmu_pagetable_master");
#endif
    mmu_attach_pagetable(&mmu_pagetable_system); /* Add L2 pte into L1 mpt */
#if configDEBUG >= KERROR_DEBUG
    KERROR(KERROR_DEBUG, "Attached mmu_pagetable_system");
#endif

    SUBSYS_INITFINI("ptmapper OK");
}

int ptmapper_alloc(mmu_pagetable_t * pt)
{
    size_t block;
    size_t addr;
    size_t size = 0; /* Size in bitmap */
    size_t bsize = 0; /* Size in bytes */
    size_t balign;
    int retval = 0;

    switch (pt->type) {
    case MMU_PTT_MASTER:
        size = PTM_MASTER;
        bsize = MMU_PTSZ_MASTER;
        balign = 0x10; /* TODO Depends on HW */
        break;
    case MMU_PTT_COARSE:
        size = PTM_COARSE;
        bsize = MMU_PTSZ_COARSE;
        balign = 0x1;
        break;
    default:
        panic("pt size can't be zero");
    }

    /* Try to allocate a new page table */
    if (!PTM_ALLOC(&block, size, balign)) {
#if configDEBUG != 0
        char buf[80];
#endif

        addr = PTM_BLOCK2ADDR(block);
#if configDEBUG >= KERROR_DEBUG
        ksprintf(buf, sizeof(buf),
                "Alloc pt %u bytes @ %x", bsize, addr);
        KERROR(KERROR_DEBUG, buf);
#endif
        pt->pt_addr = addr;
        if (pt->type == MMU_PTT_MASTER) {
            pt->master_pt_addr = addr;
        }

        /* Accounting for sysctl */
        ptm_nr_pt++;
        ptm_mem_free -= bsize;
    } else {
#if configDEBUG >= KERROR_ERR
        KERROR(KERROR_ERR, "Out of pt memory");
#endif
        retval = -1;
    }

    return retval;
}

void ptmapper_free(mmu_pagetable_t * pt)
{
    size_t block;
    size_t size = 0; /* Size in bitmap */
    size_t bsize = 0; /* Size in bytes */

    switch (pt->type) {
    case MMU_PTT_MASTER:
        size = PTM_MASTER;
        bsize = MMU_PTSZ_MASTER;
        break;
    case MMU_PTT_COARSE:
        size = PTM_COARSE;
        bsize = MMU_PTSZ_COARSE;
        break;
    default:
        KERROR(KERROR_ERR, "Attemp to free an invalid page table.");
        return;
    }

    block = PTM_ADDR2BLOCK(pt->pt_addr);
    PTM_FREE(block, size);

    /* Accounting for sysctl */
    ptm_nr_pt--;
    ptm_mem_free += bsize;

}
