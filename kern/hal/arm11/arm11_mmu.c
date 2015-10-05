/**
 *******************************************************************************
 * @file    arm11_mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control functions for ARM11 ARMv6 instruction set.
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

#include <errno.h>
#include <kstring.h>
#include <kerror.h>
#include <klocks.h>
#include <proc.h>
#include <hal/core.h>
#include <hal/mmu.h>

#ifdef configMP
static mtx_t mmu_lock;
#define MMU_LOCK() mtx_spinlock(&mmu_lock)
#define MMU_UNLOCK() mtx_unlock(&mmu_lock)
#else /* !configMP */
#define MMU_LOCK()
#define MMU_UNLOCK()
#endif /* configMP */

#define mmu_disable_ints() __asm__ volatile ("cpsid if")

#ifdef configMP
void mmu_lock_init(void);

void mmu_lock_init(void)
{
    mtx_init(&mmu_lock, MTX_TYPE_SPIN, 0);
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
    const size_t nr_tables = pt->nr_tables;
    const uint32_t pte = MMU_PTE_FAULT;
    uint32_t * p_pte = (uint32_t *)pt->pt_addr; /* points to a pt entry in PT */

    KASSERT(nr_tables > 0, "nr_tables must be greater than zero");

    if (!p_pte) {
        KERROR(KERROR_ERR, "Page table address can't be null.\n");

        return -EINVAL;
    }

    switch (pt->pt_type) {
    case MMU_PTT_COARSE:
        i = nr_tables * MMU_PTSZ_COARSE / 4 / 32; break;
    case MMU_PTT_MASTER:
        i = nr_tables * MMU_PTSZ_MASTER / 4 / 32; break;
    default:
        KERROR(KERROR_ERR, "Unknown page table type.\n");
        return -EINVAL;
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
 * Map a section of physical memory in multiples of 1 MB in virtual memory.
 * @param region    Structure that specifies the memory region.
 */
static void mmu_map_section_region(const mmu_region_t * region)
{
    int i;
    uint32_t * p_pte;
    uint32_t pte;
    const int pages = region->num_pages - 1;
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += region->vaddr >> 20;            /* Set to first pte in region */
    p_pte += pages;                          /* Set to last pte in region */

    pte = region->paddr & 0xfff00000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 10;        /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 13;        /* Set access permissions (APX) */
    pte |= (region->pt->pt_dom & 0x7) << 5; /* Set domain */
    pte |= (region->control & 0x3) << 16;   /* Set nG & S bits */
    pte |= (region->control & 0x10);        /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) << 5;  /* Set TEX bits */
    pte |= MMU_PTE_SECTION;                 /* Set entry type */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (i = pages; i >= 0; i--) {
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
static void mmu_map_coarse_region(const mmu_region_t * region)
{
    uint32_t * p_pte;
    uint32_t pte;
    const int pages = region->num_pages - 1;
    istate_t s;

    /* Page table base address */
    p_pte  = (uint32_t *)region->pt->pt_addr;
    p_pte += (region->vaddr & 0xff000) >> 12;   /* First */
    p_pte += pages;                             /* Last pte */

    KASSERT(p_pte, "p_pte not null");

    pte = region->paddr & 0xfffff000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 4;         /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 7;         /* Set access permissions (APX) */
    pte |= (region->control & 0x3) << 10;   /* Set nG & S bits */
    pte |= (region->control & 0x10) >> 4;   /* Set XN bit */
    pte |= (region->control & 0x60) >> 3;   /* Set C & B bits */
    pte |= (region->control & 0x380) >> 1;  /* Set TEX bits */
    pte |= 0x2;                             /* Set entry type (4 kB page) */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (int i = pages; i >= 0; i--) {
        *p_pte-- = pte + (i << 12); /* i = 4 KB small page */
    }

    cpu_invalidate_caches();
    set_interrupt_state(s);
    MMU_UNLOCK();
}

/**
 * Map memory region.
 * @param region    Structure that specifies the memory region.
 * @return  Zero if succeed; non-zero error code otherwise.
 */
int mmu_map_region(const mmu_region_t * region)
{
    KASSERT(region->pt != NULL, "region->pt is set");
    KASSERT(region->num_pages > 0, "num_pages must be greater than zero");

    switch (region->pt->pt_type) {
    case MMU_PTT_MASTER:    /* Map section in L1 page table */
        mmu_map_section_region(region);
        break;
    case MMU_PTT_COARSE:    /* Map PTE to point to a L2 coarse page table */
        mmu_map_coarse_region(region);
        break;
    default:
        KERROR(KERROR_ERR, "Invalid mmu_region struct.\n");
        return -EINVAL;
    }

    return 0;
}

/**
 * Unmap section pt entry region.
 * @param region    Original descriptor structure for the region.
 */
static void mmu_unmap_section_region(const mmu_region_t * region)
{
    uint32_t * p_pte;
    const uint32_t pte = MMU_PTE_FAULT;
    const uint32_t pages = region->num_pages - 1;
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr;    /* Page table base address */
    p_pte += region->vaddr >> 20;               /* Set to first pte in region */
    p_pte += pages;                             /* Set to last pte in region */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (int i = pages; i >= 0; i--) {
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
static void mmu_unmap_coarse_region(const mmu_region_t * region)
{
    uint32_t * p_pte;
    const uint32_t pte = MMU_PTE_FAULT;
    const uint32_t pages = region->num_pages - 1;
    istate_t s;

    /* Page table base address */
    p_pte  = (uint32_t *)region->pt->pt_addr;
    p_pte += (region->vaddr & 0x000ff000) >> 12;    /* First */
    p_pte += pages;                                 /* Last pte */

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (int i = pages; i >= 0; i--) {
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
int mmu_unmap_region(const mmu_region_t * region)
{
    switch (region->pt->pt_type) {
    case MMU_PTE_SECTION:
        mmu_unmap_section_region(region);
        break;
    case MMU_PTE_COARSE:
        mmu_unmap_coarse_region(region);
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static void attach_coarse_pagetable(const mmu_pagetable_t * restrict pt)
{
    uint32_t * ttb;

    KASSERT(pt->nr_tables > 0, "nr_tables must be greater than zero");

    ttb = (uint32_t *)pt->master_pt_addr;

    for (size_t j = 0; j < pt->nr_tables; j++) {
        uint32_t pte;
        size_t i;

        pte = ((pt->pt_addr + j * MMU_PTSZ_COARSE) & 0xfffffc00);
        pte |= pt->pt_dom << 5;
        pte |= MMU_PTE_COARSE;

        i = (pt->vaddr + j * MMU_PGSIZE_SECTION) >> 20;
        ttb[i] = pte;
    }
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
    istate_t s;
    int retval = 0;

    ttb = (uint32_t *)(pt->master_pt_addr);

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    switch (pt->pt_type) {
    case MMU_PTT_MASTER:
        /* TTB -> CP15:c2:c0,0 : TTBR0 */
        __asm__ volatile (
            "MCR p15, 0, %[ttb], c2, c0, 0"
            :
            : [ttb]"r" (ttb));

        break;
    case MMU_PTT_COARSE:
        /* First level coarse page table entry */
        attach_coarse_pagetable(pt);
        break;
    default:
        retval = -EINVAL;
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
int mmu_detach_pagetable(const mmu_pagetable_t * pt)
{
    uint32_t * ttb;
    const size_t nr_tables = pt->nr_tables;
    uint32_t i, j;
    istate_t s;

    KASSERT(nr_tables > 0, "nr_tables must be greater than zero");

    if (pt->pt_type == MMU_PTT_MASTER) {
        KERROR(KERROR_ERR, "Cannot detach a master pt\n");

        return -EPERM;
    }

    /* Mark page table entry at i as translation fault */
    ttb = (uint32_t *)pt->master_pt_addr;

    MMU_LOCK();
    s = get_interrupt_state();
    mmu_disable_ints();

    for (j = 0; j < nr_tables; j++) {
        i = (pt->vaddr + j * MMU_PGSIZE_SECTION) >> 20;
        ttb[i] = MMU_PTE_FAULT;
    }

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
 * Translate a vaddr to a physical address.
 */
void * mmu_translate_vaddr(const mmu_pagetable_t * pt, uintptr_t vaddr)
{
    void * retval = 0;
    uint32_t * p_pte;
    uint32_t mask;
    size_t page_size;
    size_t offset = (size_t)(vaddr - (uintptr_t)(pt->vaddr));

    switch (pt->pt_type) {
    case MMU_PTT_MASTER:
        page_size = MMU_PGSIZE_SECTION;
        mask    = 0xfff00000;
        offset &= 0x000fffff;
        p_pte   = (uint32_t *)pt->pt_addr; /* Page table base address */
        p_pte  += vaddr >> 20; /* Set to the corresponding pte */
        break;
    case MMU_PTT_COARSE:
        page_size = MMU_PGSIZE_COARSE;
        mask    = 0xfffff000;
        offset &= 0x00000fff;
        p_pte   = (uint32_t *)pt->pt_addr;
        p_pte  += (vaddr & 0x000ff000) >> 12;
        break;
    default:
        KERROR(KERROR_ERR, "Invalid pt type.\n");
        goto out;
    }

    if (offset > page_size) {
        goto out;
    } else {
        retval = (void *)(((uintptr_t)(*p_pte) & mask) + offset);
    }

out:
    return retval;
}

void arm11_abo_dump(const struct mmu_abo_param * restrict abo)
{
    const char * const abo_type_str = (abo->abo_type == MMU_ABO_DATA) ? "DAB"
                                                                      : "PAB";

    KERROR(KERROR_CRIT,
           "Fatal %s:\n"
           "pc: %x\n"
           "(i)fsr: %x (%s)\n"
           "(i)far: %x\n"
           "proc info:\n"
           "pid: %i\n"
           "tid: %i\n"
           "insys: %i\n",
           abo_type_str,
           abo->lr,
           abo->fsr,
           (abo->abo_type == MMU_ABO_DATA) ? get_dab_strerror(abo->fsr)
                                           : get_pab_strerror(abo->fsr),
           abo->far,
           (int32_t)((abo->proc) ? abo->proc->pid : -1),
           (int32_t)abo->thread->id,
           (int32_t)thread_flags_is_set(abo->thread, SCHED_INSYS_FLAG));
    stack_dump(abo->thread->sframe[SCHED_SFRAME_ABO]);
}

int arm11_abo_buser(const struct mmu_abo_param * restrict abo)
{
    const struct ksignal_param sigparm = {
        .si_code = SEGV_MAPERR,
        .si_addr = (void *)abo->far,
    };

    /* Some cases are always fatal */
    if (!ABO_WAS_USERMODE(abo->psr) /* it happened in kernel mode */ ||
        (abo->thread->pid_owner <= 1)) /* the proc is kernel or init */ {
        return -ENOTRECOVERABLE;
    }

    if (!abo->proc)
        return -ESRCH;

    KERROR(KERROR_DEBUG, "%s: Send a fatal SIGSEGV to %d\n",
           __func__, abo->proc->pid);

    /* Deliver SIGSEGV. */
    ksignal_sendsig_fatal(abo->proc, SIGSEGV, &sigparm);
    mmu_die_on_fatal_abort();

    return 0;
}
