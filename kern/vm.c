/**
 *******************************************************************************
 * @file    vm.c
 * @author  Olli Vanhoja
 * @brief   VM functions.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <libkern.h>
#include <kstring.h>
#include <hal/mmu.h>
#include <ptmapper.h>
#include <dynmem.h>
#include <kmalloc.h>
#include <kerror.h>
#include <errno.h>
#include <proc.h>
#include <buf.h>
#include <vm/vm.h>

/*
 * TODO
 * Add configHAVE_HW_PAGETABLES or similar config flag and implement a default
 * lookup function for uaddr to kaddr that doesn't need HAL support.
 */

extern mmu_region_t mmu_region_kernel;

static int test_ap_priv(uint32_t rw, uint32_t ap);
static int test_ap_user(uint32_t rw, uint32_t mmu_ap, uint32_t mmu_control);

RB_GENERATE(ptlist, vm_pt, entry_, ptlist_compare);

int ptlist_compare(struct vm_pt * a, struct vm_pt * b)
{
    const ptrdiff_t vaddr_a = (a) ? (ptrdiff_t)(a->pt.vaddr) : 0;
    const ptrdiff_t vaddr_b = (b) ? (ptrdiff_t)(b->pt.vaddr) : 0;

    return (int)(vaddr_a - vaddr_b);
}

struct vm_pt * ptlist_get_pt(struct vm_mm_struct * mm, uintptr_t vaddr)
{
    struct ptlist * const ptlist_head = &mm->ptlist_head;
    mmu_pagetable_t * const mpt = &mm->mpt;
    struct vm_pt * vpt = 0;
    struct vm_pt filter = {
        .pt.vaddr = MMU_CPT_VADDR(vaddr)
    }; /* Used as a search filter */

    KASSERT(mpt, "mpt can't be null");

    /*
     * Check if the requested page table is actually the system pagetable.
     */
    if (vaddr <= MMU_PGSIZE_SECTION - 1) {
        return &vm_pagetable_system;
    }

    /* Look for existing page table. */
    if (!RB_EMPTY(ptlist_head)) {
        vpt = RB_FIND(ptlist, ptlist_head, &filter);
    }
    if (!vpt) { /* Create a new pt if a sufficient pt not found. */
        int err;

        vpt = kcalloc(1, sizeof(struct vm_pt));
        if (!vpt) {
            return NULL;
        }

        vpt->pt.vaddr = filter.pt.vaddr;
        vpt->pt.master_pt_addr = mpt->pt_addr;
        vpt->pt.type = MMU_PTT_COARSE;
        vpt->pt.dom = MMU_DOM_USER;

        /* Allocate the actual page table, this will also set pt_addr. */
        if (ptmapper_alloc(&(vpt->pt))) {
            kfree(vpt);
            return NULL;
        }

        /* Insert vpt (L2 page table) to the new new process. */
        RB_INSERT(ptlist, ptlist_head, vpt);
        err = mmu_attach_pagetable(&(vpt->pt));
        if (err) {
            panic("Can't attach new pt");
        }
    }

    return vpt;
}

void ptlist_free(struct ptlist * ptlist_head)
{
    struct vm_pt * var, * nxt;

    if (!RB_EMPTY(ptlist_head)) {
        for (var = RB_MIN(ptlist, ptlist_head); var != 0;
                var = nxt) {
            nxt = RB_NEXT(ptlist, ptlist_head, var);
            ptmapper_free(&(var->pt));
            kfree(var);
        }
    }
}

int copyin(const void * uaddr, void * kaddr, size_t len)
{
    return copyin_proc(curproc, uaddr, kaddr, len);
}

int copyin_proc(struct proc_info * proc, const void * uaddr, void * kaddr,
        size_t len)
{
    struct vm_pt * vpt;
    void * phys_uaddr;

    if (!useracc_proc(uaddr, len, proc, VM_PROT_READ)) {
        return -EFAULT;
    }

    vpt = ptlist_get_pt(&curproc->mm, (uintptr_t)uaddr);
    if (!vpt)
        panic("Can't copyin()");

    phys_uaddr = mmu_translate_vaddr(&(vpt->pt), (uintptr_t)uaddr);
    if (!phys_uaddr) {
        return -EFAULT;
    }

    memcpy(kaddr, phys_uaddr, len);
    return 0;
}

int copyout(const void * kaddr, void * uaddr, size_t len)
{
    return copyout_proc(curproc, kaddr, uaddr, len);
}

int copyout_proc(struct proc_info * proc, const void * kaddr, void * uaddr,
        size_t len)
{
    /* TODO Handle possible cow flag? */
    struct vm_pt * vpt;
    void * phys_uaddr;

    if (!useracc_proc(uaddr, len, proc, VM_PROT_WRITE)) {
        return -EFAULT;
    }

    vpt = ptlist_get_pt(&proc->mm, (uintptr_t)uaddr);
    if (!vpt)
        panic("Can't copyout()");

    phys_uaddr = mmu_translate_vaddr(&(vpt->pt), (uintptr_t)uaddr);
    if (!phys_uaddr)
        return -EFAULT;

    memcpy(phys_uaddr, kaddr, len);
    return 0;
}

int copyinstr(const char * uaddr, char * kaddr, size_t len, size_t * done)
{
    uintptr_t last_prefix = UINTPTR_MAX;
    char * phys_uaddr;
    size_t off = 0;

    KASSERT(uaddr != NULL, "uaddr shall be set");
    KASSERT(kaddr != NULL, "kaddr shall be set");

    while (off < len) {
        if (((uintptr_t)uaddr >> NBITS(MMU_PGSIZE_COARSE)) != last_prefix) {
            struct vm_pt * vpt;

            if (!useracc(uaddr, 1, VM_PROT_READ)) {
                return -EFAULT;
            }

            vpt = ptlist_get_pt(&curproc->mm, (uintptr_t)uaddr);
            if (!vpt)
                panic("Can't copyinstr()");

            last_prefix = (uintptr_t)uaddr >> NBITS(MMU_PGSIZE_COARSE);

            phys_uaddr = mmu_translate_vaddr(&(vpt->pt), (uintptr_t)uaddr);
            if (!phys_uaddr) {
                return -EFAULT;
            }
        }

        kaddr[off] = *phys_uaddr++;
        uaddr += ++off;

        if (kaddr[off - 1] == '\0')
            break;
    }

    *done = off;

    if (kaddr[off - 1] != '\0') {
        return -ENAMETOOLONG;
    }

    return 0;
}

int vm_find_reg(struct proc_info * proc, uintptr_t uaddr, struct buf ** bp)
{
    struct buf * region = NULL;
    struct vm_mm_struct * mm = &proc->mm;
    uintptr_t reg_start, reg_end;

    mtx_lock(&mm->regions_lock);
    for (size_t i = 0; i < mm->nr_regions; i++) {
        region = (*mm->regions)[i];
        if (!region)
            continue;

        reg_start = region->b_mmu.vaddr;
        reg_end = region->b_mmu.vaddr + MMU_SIZEOF_REGION(&region->b_mmu);

        if (VM_ADDR_IS_IN_RANGE(uaddr, reg_start, reg_end)) {
            mtx_unlock(&mm->regions_lock);
            *bp = region;
            return i;
        }
    }
    mtx_unlock(&mm->regions_lock);

    return -1;
}

struct buf * vm_newsect(uintptr_t vaddr, size_t size, int prot)
{
    /*
     * We have to make the section slightly bigger than requested if vaddr
     * and vaddr + size doesn't align nicely with page boundaries.
     */
    const uintptr_t start_vaddr = (vaddr & ~(MMU_PGSIZE_COARSE - 1));
    const size_t sectsize = (vaddr + size) - start_vaddr;
    struct buf * new_region;

    new_region = geteblk(sectsize);
    if (!new_region)
        return NULL;

    new_region->b_uflags = prot & ~VM_PROT_COW;
    new_region->b_mmu.vaddr = start_vaddr;
    new_region->b_mmu.control = MMU_CTRL_MEMTYPE_WB;
    vm_updateusr_ap(new_region);

    return new_region;
}

struct buf * vm_rndsect(struct proc_info * proc, size_t size, int prot,
        struct buf * old_bp)
{
    size_t nr_regions;
    const size_t bits = NBITS(MMU_PGSIZE_COARSE);
    const uintptr_t addr_min = configEXEC_BASE_LIMIT;
    uintptr_t addr_max = 0xEFFFFFFF; /* TODO ??? */
    int overlap;
    uintptr_t vaddr;
    struct buf * bp;
    int err;

    if (old_bp)
        size = MMU_SIZEOF_REGION(&old_bp->b_mmu);

    mtx_lock(&proc->mm.regions_lock);
    nr_regions = proc->mm.nr_regions;
    do {
        uintptr_t newreg_end;

        vaddr = addr_min +
                (kunirand((addr_max >> bits) - (addr_min >> bits)) << bits);
        vaddr &= ~(MMU_PGSIZE_COARSE - 1);
        newreg_end = vaddr + size;

        overlap = 0;

        for (size_t i = 0; i < nr_regions; i++) {
            uintptr_t reg_start, reg_end;
            struct buf * region = (*proc->mm.regions)[i];
            if (!region)
                continue;

            reg_start = region->b_mmu.vaddr;
            reg_end = region->b_mmu.vaddr + MMU_SIZEOF_REGION(&region->b_mmu);

            if (VM_RANGE_IS_OVERLAPPING(reg_start, reg_end,
                                        vaddr, newreg_end)) {
                overlap = 1;
                break;
            }
        }

        if (overlap == 0)
            break;
    } while (1); /* TODO What if there is no space left? */
    mtx_unlock(&proc->mm.regions_lock);

    if (old_bp) {
        bp = old_bp;
        bp->b_mmu.vaddr = vaddr;
    } else {
        bp = vm_newsect(vaddr, size, prot);
    }
    err = vm_insert_region(proc, bp, VM_INSOP_SET_PT | VM_INSOP_MAP_REG);
    if (err < 0)
        panic("Failed to insert region"); /* TODO Handle error */

    return bp;
}

void vm_updateusr_ap(struct buf * region)
{
    int usr_rw;
    unsigned ap;

    mtx_lock(&region->lock);
    usr_rw = region->b_uflags;
    ap = region->b_mmu.ap;

#define _COWRD (VM_PROT_COW | VM_PROT_READ)
    if ((usr_rw & _COWRD) == _COWRD) {
        region->b_mmu.ap = MMU_AP_RORO;
    } else if (usr_rw & VM_PROT_WRITE) {
        region->b_mmu.ap = MMU_AP_RWRW;
    } else if (usr_rw & VM_PROT_READ) {
        switch (ap) {
        case MMU_AP_NANA:
        case MMU_AP_RONA:
            region->b_mmu.ap = MMU_AP_RORO;
            break;
        case MMU_AP_RWNA:
            region->b_mmu.ap = MMU_AP_RWRO;
            break;
        case MMU_AP_RWRO:
            break;
        case MMU_AP_RWRW:
            region->b_mmu.ap = MMU_AP_RWRO;
            break;
        case MMU_AP_RORO:
            break;
        }
    } else {
        switch (ap) {
        case MMU_AP_NANA:
        case MMU_AP_RONA:
        case MMU_AP_RWNA:
            break;
        case MMU_AP_RWRO:
        case MMU_AP_RWRW:
            region->b_mmu.ap = MMU_AP_RWNA;
            break;
        case MMU_AP_RORO:
            region->b_mmu.ap = MMU_AP_RONA;
            break;
        }
    }
#undef _COWRD

    mtx_unlock(&region->lock);
}

int realloc_mm_regions(struct vm_mm_struct * mm, int new_count)
{
    struct buf * (*new_regions)[];
    size_t i = mm->nr_regions;

    /* TODO Remove me */
    KERROR(KERROR_DEBUG,
           "realloc_mm_regions(mm %p, new_count %d), old %d\n",
           mm, new_count, i);

    if (i == 0) {
        /* TODO ticket lock? */
        mtx_init(&mm->regions_lock, MTX_TYPE_SPIN);
    }
    mtx_lock(&mm->regions_lock);

    if (new_count <= i) {
        KERROR(KERROR_DEBUG,
               "realloc_mm_regions cancelled %d <= %d\n",
               new_count, i);

        return 0;
    }

    new_regions = krealloc(mm->regions, new_count * sizeof(struct buf *));
    if (!new_regions)
        return -ENOMEM;

    for (; i < new_count; i++) {
        (*new_regions)[i] = NULL;
    }

    mm->regions = new_regions;
    mm->nr_regions = new_count;

    mtx_unlock(&mm->regions_lock);

    return 0;
}

/* TODO Verify that the mapping will be valid */
/**
 * Insert a reference to a region but don't map it.
 * @note region pt is not updated.
 * @param region is the region to be inserted, can be null to allow allocations
 *               from regions array.
 * @returns value >= 0 succeed and value is the region nr;
 *          value < 0 failed with an error code.
 */
static int vm_insert_region_ref(struct vm_mm_struct * mm, struct buf * region)
{
    size_t nr_regions = mm->nr_regions;
    int slot = -1, err;

    mtx_lock(&mm->regions_lock);
    for (size_t i = 0; i < nr_regions; i++) {
        if ((*mm->regions)[i] == NULL) {
            slot = i;
            break;
        }
    }
    mtx_unlock(&mm->regions_lock);

    if (slot == -1) {
        slot = nr_regions;
        err = realloc_mm_regions(mm, nr_regions + 1);
        if (err)
            return err;
    }

    mtx_lock(&mm->regions_lock);
    (*mm->regions)[slot] = region;
    mtx_unlock(&mm->regions_lock);

    return slot;
}

int vm_insert_region(struct proc_info * proc, struct buf * region, int op)
{
    int slot, err;

    KASSERT(region, "Region must be set");

    slot = vm_insert_region_ref(&proc->mm, NULL);
    if (slot < 0)
        return slot;

    err = vm_replace_region(proc, region, slot, op);
    if (err)
        return err;

    return slot;
}

int vm_replace_region(struct proc_info * proc, struct buf * region,
                      int region_nr, int op)
{
    struct vm_mm_struct * const mm = &proc->mm;
    struct vm_pt * vpt;
    struct buf * old_region;
    int err;

    /*
     * Realloc if necessary.
     */
    if (region_nr >= mm->nr_regions) {
        err = realloc_mm_regions(mm, region_nr + 1);
        if (err)
            return err;
    }

    mtx_lock(&mm->regions_lock);
    old_region = (*mm->regions)[region_nr];
    (*mm->regions)[region_nr] = NULL;
    mtx_unlock(&mm->regions_lock);

    if (old_region) {
        /*
         * TODO This is not the best solution but we don't want to unmap static
         * kernel regions from the process.
         */
        if (old_region->b_mmu.vaddr != mmu_region_kernel.vaddr &&
            old_region->b_mmu.vaddr != mmu_region_kdata.vaddr) {
            vm_unmapproc_region(proc, old_region);
        }
    }

    /*
     * Free the old region as this process no longer uses it.
     * (Usually decrements some internal refcount)
     */
    if (((op & VM_INSOP_NOFREE) == 0) && old_region && old_region->vm_ops &&
        old_region->vm_ops->rfree) {
        old_region->vm_ops->rfree(old_region);
    }

    /*
     * Get & set page table.
     */
    if (op & VM_INSOP_SET_PT) {
        vpt = ptlist_get_pt(mm, region->b_mmu.vaddr);
        if (!vpt)
            return -ENOMEM;

        region->b_mmu.pt = &vpt->pt;
    }

    err = 0;
    const int mask = VM_INSOP_SET_PT | VM_INSOP_MAP_REG;
    if ((op & mask) == mask) {
        err = vm_map_region(region, vpt);
    } else if (op & VM_INSOP_MAP_REG) {
        err = vm_mapproc_region(proc, region);
    }
    if (err)
        return err;

    mtx_lock(&mm->regions_lock);
    (*mm->regions)[region_nr] = region;
    mtx_unlock(&mm->regions_lock);

    /* TODO Hide debugging message with kconfig */
    KERROR(KERROR_DEBUG, "Mapped sect %d to %x (phys:%x)\n",
             region_nr, region->b_mmu.vaddr, region->b_mmu.paddr);

    return 0;
}

int vm_map_region(struct buf * region, struct vm_pt * pt)
{
    mmu_region_t mmu_region;

    KASSERT(region, "region can't be null\n");

    vm_updateusr_ap(region);
    mtx_lock(&region->lock);

    mmu_region = region->b_mmu; /* Make a copy. */
    mmu_region.pt = &(pt->pt);

    mtx_unlock(&region->lock);

    return mmu_map_region(&mmu_region);
}

int vm_mapproc_region(struct proc_info * proc, struct buf * region)
{
    struct vm_pt * vpt;

    vpt = ptlist_get_pt(&proc->mm, region->b_mmu.vaddr);
    if (!vpt)
        return -ENOMEM;

    return vm_map_region(region, vpt);
}

int vm_unmapproc_region(struct proc_info * proc, struct buf * region)
{
    struct vm_pt * vpt;
    mmu_region_t mmu_region;

    mtx_lock(&region->lock);
    vpt = ptlist_get_pt(&proc->mm, region->b_mmu.vaddr);
    if (!vpt)
        panic("Can't unmap a region");

    mmu_region = region->b_mmu;
    mmu_region.pt = &(vpt->pt);
    mtx_unlock(&region->lock);

    return mmu_unmap_region(&mmu_region);
}

int vm_unload_regions(struct proc_info * proc, int start, int end)
{
    struct vm_mm_struct * const mm = &proc->mm;

    mtx_lock(&mm->regions_lock);
    if (start < 0 || start >= mm->nr_regions || end >= mm->nr_regions) {
        mtx_unlock(&proc->mm.regions_lock);
        return -EINVAL;
    }
    if (end == -1) {
        end = mm->nr_regions - 1;
    }
    mtx_unlock(&proc->mm.regions_lock);

    for (size_t i = start; i < end; i++) {
        mtx_lock(&mm->regions_lock);
        struct buf * region = (*mm->regions)[i];
        if (!region)
            continue;

        mtx_unlock(&proc->mm.regions_lock);
        vm_replace_region(proc, NULL, i, 0);
    }

    return 0;
}

int kernacc(const void * addr, int len, int rw)
{
    size_t reg_start, reg_size;
    uint32_t ap;

    reg_start = mmu_region_kernel.vaddr;
    reg_size = mmu_sizeof_region(&mmu_region_kernel);
    if (((size_t)addr >= reg_start) && ((size_t)addr <= reg_start + reg_size))
        return (1 == 1);

    /* TODO Check other static regions as well */

    if ((ap = dynmem_acc(addr, len))) {
        if (test_ap_priv(rw, ap)) {
            return (1 == 1);
        }
    }

#if (configDEBUG >= KERROR_DEBUG)
    KERROR(KERROR_WARN,
           "Can't fully verify access to address (%p) in kernacc()\n", addr);
#endif

    return (1 == 1);
}

/**
 * Test for priv mode access permissions.
 *
 * AP format for this function:
 * 3  2    0
 * +--+----+
 * |XN| AP |
 * +--+----+
 */
static int test_ap_priv(uint32_t rw, uint32_t ap)
{
    if (rw & VM_PROT_EXECUTE) {
        if (ap & 0x8)
            return 0; /* XN bit set. */
    }
    ap &= ~0x8; /* Discard XN bit. */

    if (rw & VM_PROT_WRITE) { /* Test for RWxx */
        switch (ap) {
        case MMU_AP_RWNA:
        case MMU_AP_RWRO:
        case MMU_AP_RWRW:
            return (1 == 1);
        default:
            return 0; /* No write access. */
        }
    } else if (rw & VM_PROT_READ) { /* Test for ROxx */
        switch (ap) {
        case MMU_AP_RWNA:
        case MMU_AP_RWRO:
        case MMU_AP_RWRW: /* I guess it's ok to accept if we have rw. */
        case MMU_AP_RONA:
        case MMU_AP_RORO:
            return (1 == 1);
        default:
            return 0; /* No read access. */
        }
    }

    return 0;
}

int useracc(const void * addr, size_t len, int rw)
{
    if (!curproc)
        return 0;

    return useracc_proc(addr, len, curproc, rw);
}

int useracc_proc(const void * addr, size_t len, struct proc_info * proc, int rw)
{
    struct buf * region;
    uintptr_t reg_end;

    (void)vm_find_reg(proc, (uintptr_t)addr, &region);
    if (!region)
        return 0;
    reg_end = region->b_mmu.vaddr + MMU_SIZEOF_REGION(&region->b_mmu);

    if ((uintptr_t)addr <= reg_end)
        return test_ap_user(rw, region->b_mmu.ap, region->b_mmu.control);
    return 0;
}

static int test_ap_user(uint32_t rw, uint32_t mmu_ap, uint32_t mmu_control)
{
    if (rw & VM_PROT_EXECUTE) {
        if (mmu_control & MMU_CTRL_XN)
            return 0; /* XN bit set. */
    }

    if (rw & VM_PROT_WRITE) { /* Test for xxRW */
        if (mmu_ap & MMU_AP_RWRW)
            return 1;
        else
            return 0;
    } else if (rw & VM_PROT_READ) { /* Test for xxRO */
        if ((mmu_ap & MMU_AP_RWRO) || (mmu_ap & MMU_AP_RWRW) ||
            (mmu_ap & MMU_AP_RORO))
            return 1;
        else
            return 0;
    }

    return 0;
}
