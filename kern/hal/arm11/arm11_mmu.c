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
#include "arm11.h"
#include <hal/mmu.h>

#if configMP != 0
static mtx_t mmu_lock;
#define MMU_LOCK() mtx_spinlock(&mmu_lock)
#define MMU_UNLOCK() mtx_unlock(&mmu_lock)
#else /* !configMP */
#define MMU_LOCK()
#define MMU_UNLOCK()
#endif /* configMP */

#define mmu_disable_ints() __asm__ volatile ("cpsid if")

#define FSR_MASK                0x0f

/**
 * Test if DAB came from user mode.
 */
#define DAB_WAS_USERMODE(psr)   (((psr) & PSR_MODE_MASK) == PSR_MODE_USER)

typedef int (*dab_handler)(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread);

static void mmu_map_section_region(const mmu_region_t * region);
static void mmu_map_coarse_region(const mmu_region_t * region);
static void mmu_unmap_section_region(const mmu_region_t * region);
static void mmu_unmap_coarse_region(const mmu_region_t * region);

/* Data abort handlers */
static int dab_align(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread);
static int dab_buserr(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread);
static int dab_fatal(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread);

static const char * get_dab_strerror(uint32_t fsr);

static const dab_handler data_aborts[] = {
    dab_fatal,  /* no function, reset value */
    dab_align,  /* Alignment fault */
    dab_fatal,  /* Instruction debug event */
    proc_dab_handler,          /* Access bit fault on Section */
    dab_buserr, /* ICache maintanance op fault */
    proc_dab_handler,  /* Translation Section Fault */ /* TODO not really buserr */
    proc_dab_handler,          /* Access bit fault on Page */
    proc_dab_handler,  /* Translation Page fault */ /* TODO not really buserr */
    dab_buserr, /* Precise external abort */
    dab_buserr,  /* Domain Section fault */ /* TODO not really buserr */
    dab_fatal,  /* no function */
    dab_buserr,  /* Domain Page fault */ /* TODO Not really buserr */
    dab_buserr, /* External abort on translation, first level */
    proc_dab_handler,          /* Permission Section fault */
    dab_buserr, /* External abort on translation, second level */
    proc_dab_handler           /* Permission Page fault */
};

#if configMP != 0
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
    /* TODO Transitional hack to support nr_blocks = 0 */
    const size_t nr_tables = (pt->nr_tables == 0) ? 1 : pt->nr_tables;
    const uint32_t pte = MMU_PTE_FAULT;
    uint32_t * p_pte = (uint32_t *)pt->pt_addr; /* points to a pt entry in PT */

    if (!p_pte) {
#if config_DEBUG >= KERROR_DEBUG
        KERROR(KERROR_ERR, "Page table address can't be null.\n");
#endif
        return -EINVAL;
    }

    switch (pt->type) {
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
 * Map memory region.
 * @param region    Structure that specifies the memory region.
 * @return  Zero if succeed; non-zero error code otherwise.
 */
int mmu_map_region(const mmu_region_t * region)
{
    KASSERT(region->pt != NULL, "region->pt is set");
    KASSERT(region->num_pages > 0, "num_pages must be greater than zero");

    switch (region->pt->type) {
    case MMU_PTT_MASTER:    /* Map section in L1 page table */
        mmu_map_section_region(region);
        break;
    case MMU_PTT_COARSE:    /* Map PTE to point to a L2 coarse page table */
        mmu_map_coarse_region(region);
        break;
    default:
#if configDEBUG >= KERROR_DEBUG
        KERROR(KERROR_ERR, "Invalid mmu_region struct.\n");
#endif
        return -EINVAL;
    }

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
    /* TODO Transitional hack to support nr_blocks = 0 */
    const size_t nr_tables_raw = region->pt->nr_tables;
    const size_t nr_tables = (nr_tables_raw == 0) ? 1 : nr_tables_raw;
    const int pages = (region->num_pages - 1) & (nr_tables * 4096 - 1);
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr; /* Page table base address */
    p_pte += region->vaddr >> 20;            /* Set to first pte in region */
    p_pte += pages;                          /* Set to last pte in region */

    pte = region->paddr & 0xfff00000;       /* Set physical address */
    pte |= (region->ap & 0x3) << 10;        /* Set access permissions (AP) */
    pte |= (region->ap & 0x4) << 13;        /* Set access permissions (APX) */
    pte |= (region->pt->dom & 0x7) << 5;    /* Set domain */
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
    int i;
    uint32_t * p_pte;
    uint32_t pte;
    /* TODO Transitional hack to support nr_blocks = 0 */
    const size_t nr_tables_raw = region->pt->nr_tables;
    const size_t nr_tables = (nr_tables_raw == 0) ? 1 : nr_tables_raw;
    const int pages = (region->num_pages - 1) & (nr_tables * 256 - 1);
    istate_t s;

    p_pte = (uint32_t *)region->pt->pt_addr;    /* Page table base address */
    p_pte += (region->vaddr & 0xff000) >> 12;   /* First */
    p_pte += pages;                             /* Last pte */

    KASSERT(p_pte, "p_pte is null");

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

    for (i = pages; i >= 0; i--) {
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
static void mmu_unmap_section_region(const mmu_region_t * region)
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
static void mmu_unmap_coarse_region(const mmu_region_t * region)
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

static void attach_coarse_pagetable(const mmu_pagetable_t * restrict pt)
{
    uint32_t * ttb;
    /* TODO Transitional */
    const size_t nr_tables = (pt->nr_tables == 0) ? 1 : pt->nr_tables;
    size_t i, j;

    ttb = (uint32_t *)(pt->master_pt_addr);

    for (j = 0; j < nr_tables; j++) {
        uint32_t pte;

        pte = ((pt->pt_addr + j * MMU_PTSZ_COARSE) & 0xfffffc00);
        pte |= pt->dom << 5;
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
    /* TODO Transitional */
    const size_t nr_tables = (pt->nr_tables == 0) ? 1 : pt->nr_tables;
    uint32_t i, j;
    istate_t s;

    if (pt->type == MMU_PTT_MASTER) {
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

    switch (pt->type) {
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
#if configDEBUG >= KERROR_DEBUG
        KERROR(KERROR_ERR, "Invalid pt type.\n");
#endif
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

/**
 * Data abort handler
 */
void mmu_data_abort_handler(void)
{
    uint32_t far; /*!< Fault address */
    uint32_t fsr; /*!< Fault status */
    const uint32_t spsr = current_thread->sframe[SCHED_SFRAME_ABO].psr;
    const uint32_t lr = current_thread->sframe[SCHED_SFRAME_ABO].pc;
    const istate_t s_old = spsr & PSR_INT_MASK; /*!< Old interrupt state */
    istate_t s_entry; /*!< Int state in handler entry. */
    struct thread_info * const thread = (struct thread_info *)current_thread;
    int err;

    __asm__ volatile (
        "MRC p15, 0, %[reg], c6, c0, 0"
        : [reg]"=r" (far));
    __asm__ volatile (
        "MRC p15, 0, %[reg], c5, c0, 0"
        : [reg]"=r" (fsr));

    mmu_pf_event();

    /* TODO Change master page table */

    /* TODO We may want to block the process owning this thread and possibly
     * make sure that this instance is the only one handling page fault of the
     * same kind. */

    /* Handle this data abort in pre-emptible state if possible. */
    if (DAB_WAS_USERMODE(spsr)) {
        s_entry = get_interrupt_state();
        //set_interrupt_state(s_old);
    }

    if (data_aborts[fsr & FSR_MASK]) {
        if ((err = data_aborts[fsr & FSR_MASK](fsr, far, spsr, lr, thread))) {
            /* TODO Handle this nicer... signal? */
            KERROR(KERROR_CRIT, "DAB handling failed: %i\n", err);

            stack_dump(current_thread->sframe[SCHED_SFRAME_ABO]);
            dab_fatal(fsr, far, spsr, lr, thread);
        }
    } else {
       KERROR(KERROR_CRIT,
              "DAB handling failed, no sufficient handler found.\n");

       dab_fatal(fsr, far, spsr, lr, thread);
    }

    /* TODO In the future we may wan't to support copy on read too
     * (ie. page swaping). To suppor cor, and actually anyway, we should test
     * if error appeared during reading or writing etc.
     */

    if (DAB_WAS_USERMODE(spsr)) {
        set_interrupt_state(s_entry);
    }
}

/**
 * DAB handler for alignment aborts.
 */
static int dab_align(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread)
{
    /* Kernel mode alignment fault is fatal. */
    if (!DAB_WAS_USERMODE(psr)) {
        dab_fatal(fsr, far, psr, lr, thread);
    }

    /* TODO Deliver SIGBUS */
    return -1;
}

static int dab_buserr(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread)
{
    if (!DAB_WAS_USERMODE(psr)) {
        dab_fatal(fsr, far, psr, lr, thread);
    }

    /* TODO Deliver SIGBUS */
    return -1;
}

/**
 * DAB handler for fatal aborts.
 */
static int dab_fatal(uint32_t fsr, uint32_t far, uint32_t psr, uint32_t lr,
        struct thread_info * thread)
{
    char buf[200];

    ksprintf(buf, sizeof(buf),
            "pc: %x\n"
            "fsr: %x (%s)\n"
            "far: %x\n"
            "proc info:\n"
            "pid: %u\n"
            "tid: %u\n"
            "insys: %u\n",
            lr,
            fsr, get_dab_strerror(fsr),
            far,
            current_process_id,
            thread->id,
            thread_flags_is_set(thread, SCHED_INSYS_FLAG));
    KERROR(KERROR_CRIT, "Fatal DAB\n");
    kputs(buf);
    panic("Can't handle data abort");

    return -1; /* XXX Doesn't return yet as we panic but this makes split happy */
}

static const char * dab_fsr_strerr[] = {
    /* String                       FSR[10,3:0] */
    "TLB Miss",                     /* 0x000 */
    "Alignment",                    /* 0x001 */
    "Instruction debug event",      /* 0x002 */
    "Section AP fault",             /* 0x003 */
    "Icache maintenance op fault",  /* 0x004 */
    "Section translation",          /* 0x005 */
    "Page AP fault",                /* 0x006 */
    "Page translation",             /* 0x007 */
    "Precise external abort",       /* 0x008 */
    "Domain section fault",         /* 0x009 */
    "",                             /* 0x00A */
    "Domain page fault",            /* 0x00B */
    "Extrenal first-level abort",   /* 0x00C */
    "Section permission fault",     /* 0x00D */
    "External second-evel abort",   /* 0x00E */
    "Page permission fault",        /* 0x00F */
    "",                             /* 0x010 */
    "",                             /* 0x011 */
    "",                             /* 0x012 */
    "",                             /* 0x013 */
    "",                             /* 0x014 */
    "",                             /* 0x015 */
    "Imprecise external abort",     /* 0x406 */
    "",                             /* 0x017 */
    "Parity error exception, ns",   /* 0x408 */
    "",                             /* 0x019 */
    "",                             /* 0x01A */
    "",                             /* 0x01B */
    "",                             /* 0x01C */
    "",                             /* 0x01D */
    "",                             /* 0x01E */
    ""                              /* 0x01F */
};


static const char * get_dab_strerror(uint32_t fsr)
{
    uint32_t tmp;

    tmp = fsr & 0xf;
    tmp |= (fsr & 0x400) >> 6;

    return dab_fsr_strerr[tmp];
}
