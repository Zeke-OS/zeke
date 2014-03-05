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

#include <kstring.h>
#include <kerror.h>
#include <klocks.h>
#include <proc.h>
#include "arm11.h"
#include "arm11_mmu.h"

#if configMP != 0
static mtx_t mmu_lock;
#define MMU_LOCK() mtx_spinlock(&mmu_lock)
#define MMU_UNLOCK() mtx_unlock(&mmu_lock)
#else /* !configMP */
#define MMU_LOCK()
#define MMU_UNLOCK()
#endif /* configMP */

#define mmu_disable_ints() __asm__ volatile ("cpsid if")

#define FSR_MASK 0x0f

typedef int (*dab_handler)(uint32_t fsr, uint32_t far, threadInfo_t * thread);

static void mmu_map_section_region(mmu_region_t * region);
static void mmu_map_coarse_region(mmu_region_t * region);
static void mmu_unmap_section_region(mmu_region_t * region);
static void mmu_unmap_coarse_region(mmu_region_t * region);

/* Data abort handlers */
static int dab_fatal(uint32_t fsr, uint32_t far, threadInfo_t * thread);

static const dab_handler data_aborts[] = {
    dab_fatal,  /* no function, reset value */
    dab_fatal,  /* Alignment fault */
    dab_fatal,  /* Instruction debug event */
    dab_fatal,  /* Access bit fault on Section */
    dab_fatal,  /* no function */
    dab_fatal,  /* Translation Section Fault */
    dab_fatal,  /* Access bit fault on Page */
    dab_fatal,  /* Translation Page fault */
    dab_fatal,  /* Precise external abort */
    dab_fatal,  /* Domain Section fault */
    dab_fatal,  /* no function */
    dab_fatal,  /* Domain Page fault */
    dab_fatal,  /* External abort on translation, first level */
    dab_fatal,  /* Permission Section fault */
    dab_fatal,  /* External abort on translation, second level */
    dab_fatal   /* Permission Page fault */
};

#if configMP != 0
void mmu_lock_init(void);

void mmu_lock_init(void)
{
    mtx_init(&mmu_lock, MTX_DEF | MTX_SPIN);
}
#endif

/**
 * Initialize the page table pt by filling it with FAULT entries.
 * @param pt    page table.
 * @return  0 if page table was initialized; value other than zero if page table
 *          was not initialized successfully.
 */
int mmu_init_pagetable(const mmu_pagetable_t * pt)
{
    int i;
    const uint32_t pte = MMU_PTE_FAULT;
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
#if configDEBUG
        KERROR(KERROR_ERR, "Invalid mmu_region struct.");
#endif
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
    istate_t s;

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

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 20); /* i = 1 MB section */
    }

    cpu_invalidate_caches();
    set_interrupt_state(s);
    MMU_UNLOCK();
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
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += (region->vaddr & 0x000ff000) >> 12;    /* First */
    p_pte += region->num_pages - 1;                 /* Last pte */

#if configDEBUG
        if (p_pte == 0)
            KERROR(KERROR_ERR, "p_pte is null");
#endif


    pte = region->paddr & 0xfffff000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 4;         /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 7;         /* Set access permissions (APX) */
    pte |= (region->control & 0x3) << 10;   /* Set nG & S bits */
    pte |= (region->control & 0x10) >> 2;    /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) >> 1;  /* Set TEX bits */
    pte |= 0x2;                             /* Set entry type */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 12); /* i = 4 KB small page */
    }

    cpu_invalidate_caches();
    set_interrupt_state(s);
    MMU_UNLOCK();
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
    const uint32_t pte = MMU_PTE_FAULT;
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += region->vaddr >> 20; /* Set to first pte in region */
    p_pte += region->num_pages - 1; /* Set to last pte in region */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 20); /* i = 1 MB section */
    }

    cpu_invalidate_caches();
    set_interrupt_state(s);
    MMU_UNLOCK();
}

/**
 * Unmap coarse pt entry region.
 * @param region    Original descriptor structure for the region.
 */
static void mmu_unmap_coarse_region(mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    const uint32_t pte = MMU_PTE_FAULT;
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += (region->vaddr & 0x000ff000) >> 12;    /* First */
    p_pte += region->num_pages - 1;                 /* Last pte */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (i = region->num_pages - 1; i >= 0; i--) {
        *p_pte-- = pte + (i << 12); /* i = 4 KB small page */
    }

    cpu_invalidate_caches();
    set_interrupt_state(s);
    MMU_UNLOCK();
}

/**
 * Attach a L2 page table to a L1 master page table or attach a L1 page table.
 * @param pt    A page table descriptor structure.
 * @return  Zero if attach succeed; non-zero error code if invalid page table
 *          type.
 */
int mmu_attach_pagetable(const mmu_pagetable_t * pt)
{
    uint32_t * ttb;
    uint32_t pte;
    size_t i;
    istate_t s;
    int retval = 0;

    ttb = (uint32_t *)(pt->master_pt_addr);
    if (ttb == 0) {
        char buf[200];

        ksprintf(buf, sizeof(buf),
                "pt->master_pt_addr can't be null.\n"
                "pt->vaddr = %x\npt->type = %s\npt->pt_addr = %x",
                pt->vaddr,
                (pt->type == MMU_PTT_MASTER) ? "master" : "coarse",
                pt->pt_addr);
        panic(buf);
    }

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

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
    set_interrupt_state(s);
    MMU_UNLOCK();

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
    istate_t s;

    if (pt->type == MMU_PTT_MASTER) {
        KERROR(KERROR_ERR, "Cannot detach a master pt");
        return -1;
    }

    /* Mark page table entry at i as translation fault */
    ttb = (uint32_t *)pt->master_pt_addr;
    i = pt->vaddr >> 20;

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    ttb[i] = MMU_PTE_FAULT;

    cpu_invalidate_caches();
    set_interrupt_state(s);
    MMU_UNLOCK();

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
 * Translate vaddr to physical address.
 */
void * mmu_translate_vaddr(const mmu_pagetable_t * pt, intptr_t vaddr)
{
    void * retval = 0;
    uint32_t * p_pte;
    uint32_t mask;
    size_t ptsize;
    const size_t offset = vaddr - (intptr_t)(pt->vaddr);

    switch (pt->type) {
    case MMU_PTT_MASTER:
        ptsize = MMU_PTSZ_MASTER;
        mask   = 0xfff00000;
        p_pte  = (uint32_t *)pt->pt_addr; /* Page table base address */
        p_pte += vaddr >> 20; /* Set to the first pte */
        break;
    case MMU_PTT_COARSE:
        ptsize = MMU_PTSZ_COARSE;
        mask   = 0xfffff000;
        p_pte  = (uint32_t *)pt->pt_addr;
        p_pte += (vaddr & 0x000ff000) >> 12;
        break;
    default:
#if configDEBUG
        KERROR(KERROR_ERR, "Invalid pt type.");
#endif
        goto out;
    }

    if (offset > ptsize)
        goto out;
    else retval = (void *)(((intptr_t)(*p_pte) & mask) + offset);

out:
    return retval;
}

/**
 * Data abort handler
 * @param sp        is the thread stack pointer.
 * @param spsr      is the spsr from the thread context.
 * @param retval    is a preset return value that is used by the caller.
 */
uint32_t mmu_data_abort_handler(uint32_t sp, uint32_t spsr, uint32_t retval)
{
    uint32_t far; /*!< Fault address */
    uint32_t fsr; /*!< Fault status */
    const istate_t s_old = spsr & 0x1C0; /*!< Old interrupt state */
    istate_t s_entry; /*!< Int state in handler entry. */
    const uint32_t mode_old = spsr & 0x1f;
    threadInfo_t * const thread = (threadInfo_t *)current_thread;

    __asm__ volatile (
        "MRC p15, 0, %[reg], c0, c0, 1"
        : [reg]"=r" (far));
    __asm__ volatile (
        "MRC p15, 0, %[reg], c5, c0, 0"
        : [reg]"=r" (fsr));

    mmu_pf_event();

    /* TODO We may want to block the process owning this thread and possibly
     * make sure that this instance is the only one handling page fault of the
     * same kind. */

    /* Handle this data abort in pre-emptible state if possible. */
    //if (mode_old == 0x1f || mode_old == 0x10) {
    if (mode_old == 0x10) {
        s_entry = get_interrupt_state();
        set_interrupt_state(s_old);
    }

    if (data_aborts[fsr & FSR_MASK] != 0) {
        if (data_aborts[fsr & FSR_MASK](fsr, far, thread)) {
            /* TODO Handle this nicer... signal? */
            panic("DAB handling failed");
        }
        goto out;
    } /* else normal vm related page fault. */

    /* proc should handle this page fault as it has the knowledge of how memory
     * regions are used in processes. */
    if (proc_cow_handler(thread->pid_owner, far)) {
        /* TODO We want to send a signal here */
        panic("SEGFAULT");
    }

    //if (mode_old == 0x1f || mode_old == 0x10) {
    if (mode_old == 0x10) {
        set_interrupt_state(s_entry);
    }

out:
    return retval;
}

static int dab_fatal(uint32_t fsr, uint32_t far, threadInfo_t * thread)
{
    char buf[80];

    ksprintf(buf, sizeof(buf), "Can't handle %x data abort", fsr);
    panic(buf);
}

/**
  * @}
  */

/**
  * @}
  */
