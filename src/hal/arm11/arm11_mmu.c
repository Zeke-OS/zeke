/**
 *******************************************************************************
 * @file    arm11_mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control functions for ARM11 ARMv6 instruction set.
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

/** @addtogroup ARM11
  * @{
  */

#include "arm11_mmu.h"

#define MMU_PT_BASE             0x00018000

/* TODO move to memmap */
/** Base address of Page table region */
#define MMU_VADDR_MASTER_PT     MMU_PT_BASE
#define MMU_VADDR_KERNEL_START  0x00000000
#define MMU_VADDR_SHARED_START  0x00010000

/** Size of static L1 tables */
#define MMU_PT_L1TABLES (MMU_PTSZ_MASTER)

/**
 * A macro to calculate the address for statically allocated page table.
 *
 * Note: We assume that there is only one static master table and all other
 *       tables are equally sized coarse page tables.
 */
#define MMU_PT_ADDR(index) (MMU_PT_BASE + MMU_PT_L1TABLES + index * MMU_PTSZ_COARSE)

/**
 * Last static page table index.
 */
#define MMU_PT_LAST_SINDEX 1

/**
 * First dynamic page table address.
 */
#define MMU_PT_FIRST_DYNPT MMU_PT_ADDR(MMU_PT_LAST_SINDEX + 1)


static void mmu_map_fault(mmu_region_t * region);
static void mmu_map_section(mmu_region_t * region);
static void mmu_map_coarse(mmu_region_t * region);


/* Fixed page tables */

/* Master, allocated directly on L1 */
mmu_pagetable_t mmu_pagetable_master = {
    .vaddr          = MMU_PT_BASE,
    .pt_addr        = MMU_PT_BASE,
    .master_pt_addr = MMU_PT_BASE,
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_KERNEL
};

mmu_pagetable_t mmu_pagetable_system = {
    .vaddr          = 0x0,
    .pt_addr        = MMU_PT_ADDR(1),
    .master_pt_addr = MMU_VADDR_MASTER_PT,
    .type           = MMU_PTT_COARSE,
    .dom            = MMU_DOM_KERNEL
};


/* Regions */

mmu_region_t mmu_region_kernel = {
    .vaddr          = MMU_VADDR_KERNEL_START,
    .num_pages      = 32, /* TODO Temporarily mapped as a one area */
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WB | MMU_CTRL_NG,
    .paddr          = 0x0,
    .pt             = &mmu_pagetable_system
};

mmu_region_t mmu_region_shared = {
    .vaddr          = MMU_VADDR_SHARED_START,
    .num_pages      = 4,
    .ap             = MMU_AP_RWRO,
    .control        = MMU_CTRL_MEMTYPE_WT,
    .paddr          = MMU_VADDR_SHARED_START,
    .pt             = &mmu_pagetable_system
};

mmu_region_t mmu_region_page_tables = {
    .vaddr          = MMU_PT_BASE,
    .num_pages      = 8, /* TODO 32 megs of page tables?? */
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WT,
    .paddr          = MMU_PT_BASE,
    .pt             = &mmu_pagetable_system
};


/**
 * Initialize the page table pt by filling it with FAULT entries.
 * @param pt page table.
 * @return 0 if page table was initialized; value other than zero if page table
 * was not initialized successfully.
 */
int mmu_init_pagetable(mmu_pagetable_t * pt)
{
    int i;
    uint32_t pte = MMU_PTE_FAULT;
    uint32_t * p_pte = (uint32_t *)pt->pt_addr; /* points to page table entry in PT */

    switch (pt->type) {
        case MMU_PTT_COARSE:
            i = MMU_PTSZ_COARSE/4/32; break;
        case MMU_PTT_MASTER:
            i = MMU_PTSZ_MASTER/4/32; break;
        default:
            /* Error: Unknown page table type. */
            return -1;
    }

    __asm__ volatile (
           "MOV r4, %[pte]\n\t"
           "MOV r5, %[pte]\n\t"
           "MOV r6, %[pte]\n\t"
           "MOV r7, %[pte]\n"
           "L_init_pt_top%=:\n\t"           /* for (; i != 0; i--) */
           "STMIA %[p_pte]!, {r4-r7}\n\t"   /* Write 32 entries to the table */
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
 * @param region structure that specifies the memory region.
 * @return Zero if succeed; non-zero error code otherwise.
 */
int mmu_map_region(mmu_region_t * region)
{
    switch (region->pt->type) {
        case MMU_PTE_SECTION:   /* Map section in L1 page table */
            mmu_map_section(region);
            break;
        case MMU_PTE_COARSE:    /* Map PTE to point to coarse L2 page table */
            mmu_map_coarse(region);
            break;
        case MMU_PTE_FAULT:
            /* TODO clear entries & free memory if possible (remove tables too) */
            /*break;*/
        default:
            return -1;
    }

    return 0;
}

/* static void mmu_map_fault(mmu_region_t * region)
 * TODO */

/**
 * Map a <= 1 MB section of physical memory.
 * @param region structure that specifies the memory region.
 */
static void mmu_map_section(mmu_region_t * region)
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
    pte |= (region->control & 0x4);         /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) << 3;  /* Set TEX bits */
    pte |= MMU_PTE_SECTION;                 /* Set entry type */

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 20); /* i = 1 MB section */
    }
}

/**
 * Map a <=1 MB section of physical memory to a page table.
 * @note xn bit an ap configuration is copied to all pages in this region.
 * @param region structure that specifies the memory region.
 */
static void mmu_map_coarse(mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    uint32_t pte;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += (region->vaddr & 0x000ff000) >> 12;    /* First */
    p_pte += region->num_pages -1;                  /* Last pte */

    pte = region->paddr & 0xfffff000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 4;         /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 7;         /* Set access permissions (APX) */
    pte |= (region->control & 0x3) << 10;   /* Set nG & S bits */
    pte |= (region->control & 0x4) >> 2;    /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) >> 1;  /* Set TEX bits */
    pte |= 0x2;                             /* Set entry type */

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 12); /* i = 4 KB small page */
    }
}

/**
 * Attach a L2 page table to a L1 master page table or attach a L1 page table.
 * @param pt a page table descriptor structure.
 * @return Zero if attach succeed; non-zero error code if invalid
 *         page table type.
 */
int mmu_attach_pagetable(mmu_pagetable_t * pt)
{
    uint32_t * ttb;
    uint32_t pte, i;

    ttb = (uint32_t *)pt->master_pt_addr;
    i = pt->vaddr >> 20;

    switch (pt->type) {
        case MMU_PTT_MASTER:
            /* TTB -> CP15:c2:c0 */
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

            ttb[i] = pte;
            break;
        default:
            return -1;
    }

    return 0;
}

/**
 * Detach a L2 page table from a L1 master page table.
 * @param pt a page table descriptor structure.
 * @return Zero if attach succeed; value other than zero in case of error.
 */
int mmu_detach_pagetable(mmu_pagetable_t * pt)
{
    uint32_t * ttb;
    uint32_t i;

    if (pt->type == MMU_PTT_MASTER) {
        /* Not possible to detach a master pt */
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
 * @param value contains the configuration bit fields for changed domains.
 * @param mask selects which domains are updated.
 */
void mmu_domain_access_set(uint32_t value, uint32_t mask)
{
    uint32_t acr, n;

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
 * @param value control bits.
 * @param mask control bits that will be changed.
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
 * Initialize the MMU.
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

    value = MMU_ZEKE_DEF;
    mask = MMU_ZEKE_DEF;
    mmu_control_set(value, mask);
}

/**
  * @}
  */

/**
  * @}
  */
