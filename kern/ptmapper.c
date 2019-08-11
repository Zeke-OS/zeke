/**
 *******************************************************************************
 * @file    ptmapper.c
 * @author  Olli Vanhoja
 * @brief   Page table mapper.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <sys/sysctl.h>
#include <bitmap.h>
#include <kerror.h>
#include <kinit.h>
#include <kmem.h>
#include <kstring.h>
#include <ptmapper.h>

#define PTREGION_SIZE \
    MMU_PAGE_CNT_BY_RANGE(configPT_AREA_START, configPT_AREA_END, \
                          MMU_PGSIZE_SECTION)
mmu_region_t mmu_region_page_tables = {
    .vaddr          = configPT_AREA_START,
    .num_pages      = PTREGION_SIZE,
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WT | MMU_CTRL_XN,
    .paddr          = configPT_AREA_START,
    .pt             = &mmu_pagetable_master
};
KMEM_FIXED_REGION(mmu_region_page_tables);

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

SYSCTL_DECL(_vm_ptmapper);
SYSCTL_NODE(_vm, OID_AUTO, ptmapper, CTLFLAG_RW, 0,
            "ptmapper stats");

static int ptm_nr_pt = 0;
SYSCTL_INT(_vm_ptmapper, OID_AUTO, nr_pt, CTLFLAG_RD, &ptm_nr_pt, 0,
    "Total number of page tables allocated.");

static size_t ptm_mem_free = PTREGION_SIZE * MMU_PGSIZE_SECTION;
SYSCTL_UINT(_vm_ptmapper, OID_AUTO, mem_free, CTLFLAG_RD, &ptm_mem_free, 0,
    "Amount of free page table region memory.");

static const size_t ptm_mem_tot = PTREGION_SIZE * MMU_PGSIZE_SECTION;
SYSCTL_UINT(_vm_ptmapper, OID_AUTO, mem_tot, CTLFLAG_RD,
    (unsigned int *)(&ptm_mem_tot), 0,
    "Total size of the page table region.");

mtx_t ptmapper_lock = MTX_INITIALIZER(MTX_TYPE_SPIN, MTX_OPT_DEFAULT);

#define PTM_SIZEOF_MAP sizeof(ptm_alloc_map)

/**
 * Length of a master page table in ptm_alloc_map.
 */
#define PTM_MASTER  (MMU_PTSZ_MASTER / MMU_PTSZ_COARSE)

/**
 * Length of a coarse page table in ptm_alloc_map.
 */
#define PTM_COARSE  0x01

/**
 * Convert a block index to an address.
 */
#define PTM_BLOCK2ADDR(block) (configPT_AREA_START + (block) * MMU_PTSZ_COARSE)

/**
 * Convert an address to a block index.
 */
#define PTM_ADDR2BLOCK(addr) (((addr) - configPT_AREA_START) / MMU_PTSZ_COARSE)

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
    bitmap_block_update(ptm_alloc_map, 0, block, len, PTM_SIZEOF_MAP)

int ptmapper_alloc(mmu_pagetable_t * pt)
{
    size_t block;
    size_t size = 0; /* Size in bitmap */
    size_t bsize = 0; /* Size in bytes */
    size_t balign;
    int retval = 0;

    /* TODO Transitional fix */
    if (pt->nr_tables == 0) {
        KERROR(KERROR_WARN, "Transitional fix\n");
        pt->nr_tables = 1;
    }

    switch (pt->pt_type) {
    case MMU_PTT_MASTER:
        size = pt->nr_tables * PTM_MASTER;
        bsize = pt->nr_tables * MMU_PTSZ_MASTER;
        balign = PTM_MASTER;
        break;
    case MMU_PTT_COARSE:
        size = pt->nr_tables * PTM_COARSE;
        bsize = pt->nr_tables * MMU_PTSZ_COARSE;
        balign = PTM_COARSE;
        break;
    default:
        KERROR(KERROR_ERR, "Invalid pt type");
        return -EINVAL;
    }

    /* Try to allocate a new page table */
    if (_kmem_ready)
        mtx_lock(&ptmapper_lock);
    if (!PTM_ALLOC(&block, size, balign)) {
        uintptr_t addr = PTM_BLOCK2ADDR(block);

        KERROR_DBG("Alloc pt %u bytes @ %x\n", bsize, (unsigned)addr);

        pt->pt_addr = addr;
        if (pt->pt_type == MMU_PTT_MASTER) {
            pt->master_pt_addr = addr;
        }
        mmu_init_pagetable(pt);

        /* Accounting for sysctl */
        ptm_nr_pt++;
        ptm_mem_free -= bsize;
    } else {
        KERROR(KERROR_ERR, "Out of pt memory\n");
        retval = -ENOMEM;
    }
    if (_kmem_ready)
        mtx_unlock(&ptmapper_lock);

    return retval;
}

void ptmapper_free(mmu_pagetable_t * pt)
{
    size_t block;
    size_t size; /* Size in bitmap */
    size_t bsize; /* Size in bytes */

    bsize = mmu_sizeof_pt(pt);
    size = bsize / MMU_PTSZ_COARSE;

    if (size == 0) {
        KERROR(KERROR_ERR, "Attemp to free an invalid page table.\n");
        return;
    }

    block = PTM_ADDR2BLOCK(pt->pt_addr);
    PTM_FREE(block, size);

    /* Accounting for sysctl */
    ptm_nr_pt--;
    ptm_mem_free += bsize;
}
