/**
 *******************************************************************************
 * @file    arm11_mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control functions for ARM11 ARMv6 instruction set.
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

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM11
  * @{
  */

#include <kerror.h>
#include "arm11.h"
#include "arm11_mmu.h"

/* TODO
 * - region to pt pointer null checks
 */

static void mmu_map_section_region(mmu_region_t * region);
static void mmu_map_coarse_region(mmu_region_t * region);
static void mmu_unmap_section_region(mmu_region_t * region);
static void mmu_unmap_coarse_region(mmu_region_t * region);


/**
 * Initialize the page table pt by filling it with FAULT entries.
 * @param pt    page table.
 * @return  0 if page table was initialized; value other than zero if page table
 *          was not initialized successfully.
 */
int mmu_init_pagetable(mmu_pagetable_t * pt)
{
    int i;
    uint32_t pte = MMU_PTE_FAULT;
    uint32_t * p_pte = (uint32_t *)pt->pt_addr; /* points to a pt entry in PT */

#if config_DEBUG != 0
    if (p_pte == 0) {
        KERROR(KERROR_ERR, "Page table address can't be null.");
        return -1;
    }
#endif

    switch (pt->type) {
        case MMU_PTT_COARSE:
            i = MMU_PTSZ_COARSE / 4 / 32; break;
        case MMU_PTT_MASTER:
            i = MMU_PTSZ_MASTER / 4 / 32; break;
        default:
            KERROR(KERROR_ERR, "Unknown page table type.");
            return -2;
    }

    __asm__ volatile (
        "MOV r4, %[pte]\n\t"
        "MOV r5, %[pte]\n\t"
        "MOV r6, %[pte]\n\t"
        "MOV r7, %[pte]\n"
        "L_init_pt_top%=:\n\t"          /* for (; i != 0; i--) */
        "STMIA %[p_pte]!, {r4-r7}\n\t"  /* Write 32 entries to the table */
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "STMIA %[p_pte]!, {r4-r7}\n\t"
        "SUBS %[i], %[i], #1\n\t"
        "BNE L_init_pt_top%=\n\t"
        : [i]"+r" (i), [p_pte]"+r" (p_pte)
        : [pte]"r" (pte)
        : "r4", "r5", "r6", "r7"
    );

    return 0;
}

/**
 * Map memory region.
 * @param region    Structure that specifies the memory region.
 * @return  Zero if succeed; non-zero error code otherwise.
 */
int mmu_map_region(mmu_region_t * region)
{
    int retval = 0;

    switch (region->pt->type) {
        case MMU_PTT_MASTER:    /* Map section in L1 page table */
            mmu_map_section_region(region);
            break;
        case MMU_PTT_COARSE:    /* Map PTE to point to a L2 coarse page table */
            mmu_map_coarse_region(region);
            break;
        default:
            retval = -1;
            break;
    }

    return retval;
}

/**
 * Map a section of physical memory in multiples of 1 MB in virtual memory.
 * @param region    Structure that specifies the memory region.
 */
static void mmu_map_section_region(mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    uint32_t pte;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += region->vaddr >> 20; /* Set to first pte in region */
    p_pte += region->num_pages - 1; /* Set to last pte in region */

    pte = region->paddr & 0xfff00000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 10;        /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 13;        /* Set access permissions (APX) */
    pte |= region->pt->dom << 5;            /* Set domain */
    pte |= (region->control & 0x3) << 16;   /* Set nG & S bits */
    pte |= (region->control & 0x10);        /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) << 3;  /* Set TEX bits */
    pte |= MMU_PTE_SECTION;                 /* Set entry type */

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 20); /* i = 1 MB section */
    }
}

/**
 * Map a section of physical memory over a (contiguous set of) page table(s).
 * @note xn bit an ap configuration is copied to all pages in this region.
 * @note One page table maps a 1MB of memory.
 * @param region    Structure that specifies the memory region.
 */
static void mmu_map_coarse_region(mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    uint32_t pte;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += (region->vaddr & 0x000ff000) >> 12;    /* First */
    p_pte += region->num_pages - 1;                 /* Last pte */

    pte = region->paddr & 0xfffff000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 4;         /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 7;         /* Set access permissions (APX) */
    pte |= (region->control & 0x3) << 10;   /* Set nG & S bits */
    pte |= (region->control & 0x10) >> 2;    /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) >> 1;  /* Set TEX bits */
    pte |= 0x2;                             /* Set entry type */

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 12); /* i = 4 KB small page */
    }
}

/**
 * Unmap mapped memory region.
 * @param region    Original descriptor structure for the region.
 */
int mmu_unmap_region(mmu_region_t * region)
{
    int retval = 0;

    switch (region->pt->type) {
        case MMU_PTE_SECTION:
            mmu_unmap_section_region(region);
            break;
        case MMU_PTE_COARSE:
            mmu_unmap_coarse_region(region);
            break;
        default:
            retval = -1;
    }

    return retval;
}

/**
 * Unmap section pt entry region.
 * @param region    Original descriptor structure for the region.
 */
static void mmu_unmap_section_region(mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    uint32_t pte;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += region->vaddr >> 20; /* Set to first pte in region */
    p_pte += region->num_pages - 1; /* Set to last pte in region */

    pte = MMU_PTE_FAULT;

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 20); /* i = 1 MB section */
    }
}

/**
 * Unmap coarse pt entry region.
 * @param region    Original descriptor structure for the region.
 */
static void mmu_unmap_coarse_region(mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    uint32_t pte;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += (region->vaddr & 0x000ff000) >> 12;    /* First */
    p_pte += region->num_pages - 1;                 /* Last pte */

    pte = MMU_PTE_FAULT;

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 12); /* i = 4 KB small page */
    }
}

/**
 * Attach a L2 page table to a L1 master page table or attach a L1 page table.
 * @param pt    A page table descriptor structure.
 * @return  Zero if attach succeed; non-zero error code if invalid page table
 *          type.
 */
int mmu_attach_pagetable(mmu_pagetable_t * pt)
{
    uint32_t * ttb;
    uint32_t pte;
    size_t i;
    int retval = 0;

    ttb = (uint32_t *)pt->master_pt_addr;

#if configDEBUG != 0
    if (ttb == 0)
        KERROR(KERROR_ERR, "pt->master_pt_addr can't be null");
#endif


    switch (pt->type) {
        case MMU_PTT_MASTER:
            /* TTB -> CP15:c2:c0,0 : TTBR0 */
            __asm__ volatile (
                "MCR p15, 0, %[ttb], c2, c0, 0"
                 :
                 : [ttb]"r" (ttb));

            break;
        case MMU_PTT_COARSE:
            /* First level coarse page table entry */
            pte = (pt->pt_addr & 0xfffffc00);
            pte |= pt->dom << 5;
            pte |= MMU_PTE_COARSE;

            i = pt->vaddr >> 20;
            ttb[i] = pte;

            break;
        default:
            retval = -1;
            break;
    }

    cpu_invalidate_caches();
    return retval;
}

/**
 * Detach a L2 page table from a L1 master page table.
 * @param pt    A page table descriptor structure.
 * @return  Zero if attach succeed; value other than zero in case of error.
 */
int mmu_detach_pagetable(mmu_pagetable_t * pt)
{
    uint32_t * ttb;
    uint32_t i;

    if (pt->type == MMU_PTT_MASTER) {
        KERROR(KERROR_ERR, "Cannot detach a master pt");
        return -1;
    }

    /* Mark page table entry at i as translation fault */
    ttb = (uint32_t *)pt->master_pt_addr;
    i = pt->vaddr >> 20;
    ttb[i] = MMU_PTE_FAULT;

    return 0;
}

/**
 * Read domain access bits.
 */
uint32_t mmu_domain_access_get(void)
{
    uint32_t acr;

    __asm__ volatile (
            "MRC p15, 0, %[acr], c3, c0, 0"
            : [acr]"=r" (acr));

    return acr;
}

/**
 * Set access rights for selected domains.
 *
 * Mask is selected so that 0x3 = domain 1 and 0xC is domain 2 etc.
 * @param value Contains the configuration bit fields for changed domains.
 * @param mask  Selects which domains are updated.
 */
void mmu_domain_access_set(uint32_t value, uint32_t mask)
{
    uint32_t acr;

    /* Read domain access control register cp15:c3 */
    __asm__ volatile (
        "MRC p15, 0, %[acr], c3, c0, 0"
        : [acr]"=r" (acr));

    acr &= ~mask; /* Clear the bits that will be changed. */
    acr |= value; /* Set new values */

    /* Write domain access control register */
    __asm__ volatile (
        "MCR p15, 0, %[acr], c3, c0, 0"
        : : [acr]"r" (acr));
}

/**
 * Set MMU control bits.
 * @param value Control bits.
 * @param mask  Control bits that will be changed.
 */
void mmu_control_set(uint32_t value, uint32_t mask)
{
    uint32_t reg;

    __asm__ volatile (
        "MRC p15, 0, %[reg], c1, c0, 0"
        : [reg]"=r" (reg));

    reg &= ~mask;
    reg |= value;

    __asm__ volatile (
        "MCR p15, 0, %[reg], c1, c0, 0"
        : : [reg]"r" (reg));
}

/**
 * Data abort handler
 * @param sp    is the thread stack pointer.
 * @param spsr  is the spsr from the thread context.
 */
void mmu_data_abort_handler(uint32_t sp, uint32_t spsr)
{
    uint32_t far; /*!< Fault address */
    uint32_t fsr; /*!< Fault status */

    __asm__ volatile (
        "MRC p15, 0, %[reg], c0, c0, 1"
        : [reg]"=r" (far));
    __asm__ volatile (
        "MRC p15, 0, %[reg], c5, c0, 0"
        : [reg]"=r" (fsr));

    KERROR(KERROR_DEBUG, "Can't handle data aborts yet");

    while (1);
    /* TODO */
}

/**
  * @}
  */

/**
  * @}
  */
