/**
 *******************************************************************************
 * @file    vm.c
 * @author  Olli Vanhoja
 * @brief   VM functions.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <buf.h>
#include <dynmem.h>
#include <hal/mmu.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kmem.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <ptmapper.h>
#include <vm/vm.h>

/*
 * TODO Add configHAVE_HW_PAGETABLES or similar config flag and implement
 *      a default lookup function for uaddr to kaddr that doesn't need
 *      HAL support.
 *
 * TODO Reduce the amount of locking and unlocking where possible
 */

extern mmu_region_t mmu_region_kernel;

__kernel void * vm_uaddr2kaddr(struct proc_info * proc,
                               __user const void * uaddr,
                               size_t acc_size)
{
    struct vm_pt * vpt;
    void * phys_uaddr;

    vpt = ptlist_get_pt(&proc->mm, (uintptr_t)uaddr, acc_size, VM_PT_CREAT);
    if (!vpt)
        return NULL;

    phys_uaddr = mmu_translate_vaddr(&vpt->pt, (uintptr_t)uaddr);

    return phys_uaddr;
}

int copyin(__user const void * uaddr, __kernel void * kaddr, size_t len)
{
    return copyin_proc(curproc, uaddr, kaddr, len);
}

int copyin_proc(struct proc_info * proc, __user const void * uaddr,
                __kernel void * kaddr, size_t len)
{
    void * phys_uaddr;

    if (!useracc_proc(uaddr, len, proc, VM_PROT_READ)) {
        return -EFAULT;
    }

    phys_uaddr = vm_uaddr2kaddr(proc, uaddr, len);
    if (!phys_uaddr) {
        return -EFAULT;
    }

    memcpy(kaddr, phys_uaddr, len);
    return 0;
}

int copyout(__kernel const void * kaddr, __user void * uaddr, size_t len)
{
    return copyout_proc(curproc, kaddr, uaddr, len);
}

int copyout_proc(struct proc_info * proc, __kernel const void * kaddr,
                 __user void * uaddr, size_t len)
{
    /* TODO Handle possible cow flag? */
    void * phys_uaddr;

    if (!useracc_proc(uaddr, len, proc, VM_PROT_WRITE)) {
        return -EFAULT;
    }

    phys_uaddr = vm_uaddr2kaddr(proc, uaddr, len);
    if (!phys_uaddr) {
        return -EFAULT;
    }

    memcpy(phys_uaddr, kaddr, len);
    return 0;
}

int copyinstr(__user const char * uaddr, __kernel char * kaddr, size_t len,
              size_t * done)
{
    uintptr_t last_prefix = UINTPTR_MAX;
    char * phys_uaddr = NULL;
    size_t off = 0;

    KASSERT(uaddr != NULL, "uaddr shall be set");
    KASSERT(kaddr != NULL, "kaddr shall be set");

    while (off < len) {
        if (((uintptr_t)uaddr >> NBITS(MMU_PGSIZE_COARSE)) != last_prefix) {
            if (!useracc(uaddr, 1, VM_PROT_READ)) {
                return -EFAULT;
            }

            last_prefix = (uintptr_t)uaddr >> NBITS(MMU_PGSIZE_COARSE);

            phys_uaddr = vm_uaddr2kaddr(curproc, uaddr, MMU_PGSIZE_COARSE);
            if (!phys_uaddr) {
                return -EFAULT;
            }
        }
        KASSERT(phys_uaddr != NULL, "phys_addr must be set");

        kaddr[off] = *phys_uaddr++;
        uaddr += ++off;

        if (kaddr[off - 1] == '\0')
            break;
    }

    if (done)
        *done = off;

    if (kaddr[off - 1] != '\0') {
        kaddr[off - 1] = '\0';
        return -ENAMETOOLONG;
    }

    return 0;
}

int copyoutstr(__kernel char * kaddr, __user const char * uaddr, size_t len,
               size_t * done)
{
    uintptr_t last_prefix = UINTPTR_MAX;
    char * phys_uaddr = NULL; /* Make clang --analyze happy. */
    size_t off = 0;

    KASSERT(uaddr != NULL, "uaddr shall be set");
    KASSERT(kaddr != NULL, "kaddr shall be set");

    while (off < len) {
        if (((uintptr_t)uaddr >> NBITS(MMU_PGSIZE_COARSE)) != last_prefix) {
            if (!useracc(uaddr, 1, VM_PROT_READ)) {
                return -EFAULT;
            }

            last_prefix = (uintptr_t)uaddr >> NBITS(MMU_PGSIZE_COARSE);

            phys_uaddr = vm_uaddr2kaddr(curproc, uaddr, MMU_PGSIZE_COARSE);
            if (!phys_uaddr) {
                return -EFAULT;
            }
        }

        KASSERT(phys_uaddr != NULL, "phys_uaddr should be set");
        *phys_uaddr++ = kaddr[off];
        uaddr += ++off;

        if (kaddr[off - 1] == '\0')
            break;
    }

    if (done)
        *done = off;

    if (kaddr[off - 1] != '\0') {
        kaddr[off - 1] = '\0';
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
    for (int i = 0; i < mm->nr_regions; i++) {
        region = (*mm->regions)[i];
        if (!region)
            continue;

        /*
         * TODO Would be good idea to use region size instead of mmu alloc size
         *      but before that it has to be fixed everywhere in the codebase.
         */
        reg_start = region->b_mmu.vaddr;
        reg_end = region->b_mmu.vaddr + region->b_bufsize - 1;

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

/**
 * Get a free random address in mem space of proc and ensure it's mappable.
 * @note mm must be locked.
 */
static uintptr_t rnd_addr(struct vm_mm_struct * mm, size_t size)
{
    size_t nr_regions;
    const size_t bits = NBITS(MMU_PGSIZE_SECTION);
    const uintptr_t addr_min = configEXEC_BASE_LIMIT;
    const uintptr_t addr_max = (~0) >> 1;
    uintptr_t vaddr;

    KASSERT(mtx_test(&mm->regions_lock), "mm should be locked\n");

    nr_regions = mm->nr_regions;
    do {
        uintptr_t newreg_end;

tryagain:
        vaddr = addr_min +
                (kunirand((addr_max >> bits) - (addr_min >> bits)) << bits);
        vaddr &= ~(MMU_PGSIZE_COARSE - 1);
        newreg_end = vaddr + size - 1;

        for (size_t i = 0; i < nr_regions; i++) {
            uintptr_t reg_start, reg_end;
            struct buf * bp = (*mm->regions)[i];
            if (!bp)
                continue;

            reg_start = bp->b_mmu.vaddr;
            reg_end = bp->b_mmu.vaddr + bp->b_bufsize - 1;

            if (VM_RANGE_IS_OVERLAPPING(reg_start, reg_end,
                                        vaddr, newreg_end)) {
                goto tryagain;
            }
        }

        /*
         * Create the page tables early so we know it's possible to map
         * the selected address range.
         */
        if (!ptlist_get_pt(mm, vaddr, size, VM_PT_CREAT)) {
            goto tryagain;
        }
    } while (0); /* TODO What if there is no space left? */

    return vaddr;
}

struct buf * vm_rndsect(struct proc_info * proc, size_t size, int prot,
                        struct buf * old_bp)
{
    uintptr_t vaddr;
    struct buf * bp;
    int new = 0;
    int err;

    if (old_bp && size == 0)
        size = old_bp->b_bufsize;

    mtx_lock(&proc->mm.regions_lock);
    vaddr = rnd_addr(&proc->mm, size);
    mtx_unlock(&proc->mm.regions_lock);

    if (old_bp) {
        bp = old_bp;
        bp->b_mmu.vaddr = vaddr;
    } else {
        bp = vm_newsect(vaddr, size, prot);
        if (!bp)
            return NULL;
        new = 1;
    }
    err = vm_insert_region(proc, bp, VM_INSOP_MAP_REG);
    if (err < 0) {
        if (new && bp->vm_ops->rfree)
            bp->vm_ops->rfree(bp);
        return NULL;
    }

    return bp;
}

struct buf * vm_new_userstack_curproc(size_t size)
{
    struct buf * vmstack;
    uintptr_t vaddr;

    vmstack = geteblk(size);
    if (!vmstack)
        return NULL;

    mtx_lock(&curproc->mm.regions_lock);
    vaddr = rnd_addr(&curproc->mm, vmstack->b_bufsize);

    vmstack->b_uflags = VM_PROT_READ | VM_PROT_WRITE;
    vmstack->b_mmu.vaddr = vaddr;
    vmstack->b_mmu.ap = MMU_AP_RWRW;
    vmstack->b_mmu.control = MMU_CTRL_XN;

    /*
     * Unlock mm as late as possible because there might be a race condition
     * with allocations, though it's unlikely because this function is
     * most likely only called by exec.
     */
    mtx_unlock(&curproc->mm.regions_lock);

    vm_replace_region(curproc, vmstack, MM_STACK_REGION, VM_INSOP_MAP_REG);

    return vmstack;
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

int vm_mm_init(struct vm_mm_struct * mm, int nr_regions)
{
    /* Allocate a master page table for the new process. */
    mm->mpt.vaddr = 0; /* mpt always starts from zero */
    mm->mpt.nr_tables = 1;
    mm->mpt.pt_type = MMU_PTT_MASTER;
    mm->mpt.pt_dom = MMU_DOM_USER;

    if (ptmapper_alloc(&mm->mpt))
        return -ENOMEM;

    /* Allocate an array for regions. */
    mm->regions = NULL;
    mm->nr_regions = 0;
    realloc_mm_regions(mm, nr_regions);
    if (!mm->regions)
        return -ENOMEM;

    return 0;
}

void vm_mm_destroy(struct vm_mm_struct * mm)
{
    /*
     * Free all regions
     *
     * We don't use lock here because the lock descriptor data is invalidated
     * soon and any thread trying to wait for it will break anyway, so we just
     * hope there is no-one trying to lock this process anymore. Technically
     * there shouldn't be any threads locking this struct anymore.
     */
    if (mm->regions) {
        for (int i = 0; i < mm->nr_regions; i++) {
            struct buf * region = (*mm->regions)[i];

            if (region && region->vm_ops->rfree) {
                region->vm_ops->rfree(region);
            }
        }
        mm->nr_regions = 0;

        /* Free page table list. */
        ptlist_free(&mm->ptlist_head);

        /* Free regions array. */
        kfree(mm->regions);
        mm->regions = NULL;
    }

    /* Free the mpt. */
    if (mm->mpt.pt_addr)
        ptmapper_free(&mm->mpt);
}

static int realloc_mm_regions_locked(struct vm_mm_struct * mm, int new_count)
{
    struct buf * (*new_regions)[];
    int i = mm->nr_regions;

    KERROR_DBG("realloc_mm_regions(mm %p, new_count %d), old %d\n",
               mm, new_count, i);

    if (new_count <= i) {
        KERROR(KERROR_WARN,
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

    return 0;
}

int realloc_mm_regions(struct vm_mm_struct * mm, int new_count)
{
    mtx_t * lock;
    int retval;

    lock = &mm->regions_lock;
    if (mm->nr_regions == 0) {
        mtx_init(lock, MTX_TYPE_SPIN, 0);
    }
    mtx_lock(lock);
    retval = realloc_mm_regions_locked(mm, new_count);
    mtx_unlock(lock);

    return retval;
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
    int slot = -1;

    mtx_lock(&mm->regions_lock);
    for (size_t i = 0; i < nr_regions; i++) {
        if ((*mm->regions)[i] == NULL) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        int err;

        slot = nr_regions;
        err = realloc_mm_regions_locked(mm, nr_regions + 1);
        if (err)
            return err;
    }

    (*mm->regions)[slot] = region;
    mtx_unlock(&mm->regions_lock);

    return slot;
}

int vm_insert_region(struct proc_info * proc, struct buf * region, int insop)
{
    int slot, err;

    KASSERT(region, "Region must be set");

    slot = vm_insert_region_ref(&proc->mm, NULL);
    if (slot < 0)
        return slot;

    err = vm_replace_region(proc, region, slot, insop);
    if (err)
        return err;

    return slot;
}

int vm_replace_region(struct proc_info * proc, struct buf * region,
                      int region_nr, int insop)
{
    struct vm_mm_struct * const mm = &proc->mm;
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
        mmu_region_t ** regp;
        int unmap = 1;

        /*
         * We don't want to unmap static kernel regions from the process
         * memory map.
         */
        KMEM_FOREACH(regp) {
            if (old_region->b_mmu.vaddr == (*regp)->vaddr) {
                unmap = 0;
                break;
            }
        }
        if (unmap) {
            (void)vm_unmapproc_region(proc, old_region);
        }

        /*
         * Free the old region as this process no longer uses it.
         * (Usually decrements some internal refcount)
         */
        if (((insop & VM_INSOP_NOFREE) != VM_INSOP_NOFREE) &&
            old_region->vm_ops->rfree) {
            old_region->vm_ops->rfree(old_region);
        }
    }

    if (insop & VM_INSOP_MAP_REG) {
        if (!region) {
            panic("region is not set");
        }

        err = vm_mapproc_region(proc, region);
        if (err)
            return err;
    }

    mtx_lock(&mm->regions_lock);
    (*mm->regions)[region_nr] = region;
    mtx_unlock(&mm->regions_lock);

    if (region) {
        KERROR_DBG("%s: proc %d, mapped sect %d to %p (phys:%p)\n",
                   __func__, proc->pid, region_nr,
                   (void *)region->b_mmu.vaddr,
                   (void *)region->b_mmu.paddr);
    } else {
        KERROR_DBG("%s: proc %d, Clear region %d\n", __func__,
                   proc->pid, region_nr);
    }

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

    vpt = ptlist_get_pt(&proc->mm, region->b_mmu.vaddr,
                        region->b_bufsize, VM_PT_CREAT);
    if (!vpt)
        return -ENOMEM;

    return vm_map_region(region, vpt);
}

int vm_unmapproc_region(struct proc_info * proc, struct buf * region)
{
    struct vm_pt * vpt;
    mmu_region_t mmu_region;

    mtx_lock(&region->lock);
    vpt = ptlist_get_pt(&proc->mm, region->b_mmu.vaddr,
                        region->b_bufsize, VM_PT_CREAT);
    if (!vpt) {
        KERROR(KERROR_ERR, "Can't unmap a region %p for pid %d\n",
               region, proc->pid);
        return -EINVAL;
    }

    mmu_region = region->b_mmu;
    mmu_region.pt = &(vpt->pt);
    mtx_unlock(&region->lock);

    return mmu_unmap_region(&mmu_region);
}

int vm_unload_regions(struct proc_info * proc, int start, int end)
{
    struct vm_mm_struct * const mm = &proc->mm;
    int locked, retval;

    mtx_lock(&mm->regions_lock);
    locked = 1;

    if (start < 0 || start >= mm->nr_regions || end >= mm->nr_regions) {
        retval = -EINVAL;
        goto out;
    }
    if (end == -1) {
        end = mm->nr_regions - 1;
    }

    for (int i = start; i < end; i++) {
        struct buf * region;

        if (!locked) {
            mtx_lock(&mm->regions_lock);
            locked = 1;
        }

        region = (*mm->regions)[i];
        if (region) {
            mtx_unlock(&mm->regions_lock);
            locked = 0;
            vm_replace_region(proc, NULL, i, 0);
        }
    }

    retval = 0;
out:
    if (locked)
        mtx_unlock(&mm->regions_lock);

    return retval;
}

void vm_fixmemmap_proc(struct proc_info * proc)
{
    struct vm_mm_struct * mm = &proc->mm;
    size_t nr_regions = mm->nr_regions;

    mtx_lock(&mm->regions_lock);
    for (size_t i = 0; i < nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];

        if (region) {
            vm_mapproc_region(proc, region);
        }
    }
    mtx_unlock(&mm->regions_lock);
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
static int test_ap_priv(uint32_t rw, struct dynmem_ap ap)
{
    if (rw & VM_PROT_EXECUTE && ap.xn) {
        return 0; /* XN bit set. */
    }

    if (rw & VM_PROT_WRITE) { /* Test for RWxx */
        switch (ap.ap) {
        case MMU_AP_RWNA:
        case MMU_AP_RWRO:
        case MMU_AP_RWRW:
            return (1 == 1);
        default:
            return 0; /* No write access. */
        }
    } else if (rw & VM_PROT_READ) { /* Test for ROxx */
        switch (ap.ap) {
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

int kernacc(__kernel const void * addr, int len, int rw)
{
    mmu_region_t ** regp;
    struct dynmem_ap ap;

    KMEM_FOREACH(regp) {
        size_t reg_start, reg_size;

        reg_start = mmu_region_kernel.vaddr;
        /* This is the only case where mmu_sizeof_region() is ok */
        reg_size = mmu_sizeof_region(&mmu_region_kernel);

        if (((size_t)addr >= reg_start) &&
            ((size_t)addr < reg_start + reg_size)) {
            return (1 == 1);
        }
    }

    ap = dynmem_acc(addr, len);
    if (ap.ap != 0 && test_ap_priv(rw, ap)) {
        return (1 == 1);
    }

    KERROR(KERROR_WARN,
           "Can't fully verify access to address (%p) in kernacc()\n", addr);

    return (1 == 1);
}

static int test_ap_user(uint32_t rw, struct buf * bp)
{
    uint32_t mmu_ap = bp->b_mmu.ap;
    uint32_t mmu_control = bp->b_mmu.control;
    int retval = 0;

    if (rw & VM_PROT_EXECUTE) {
        if (mmu_control & MMU_CTRL_XN) {
            return 0; /* XN bit set. */
        } else {
            retval = (1 == 1);
        }
    }

    if (rw & VM_PROT_WRITE) { /* Test for xxRW */
        if (mmu_ap & MMU_AP_RWRW) {
            return (1 == 1);
        } else {
            return 0;
        }
    } else if (rw & VM_PROT_READ) { /* Test for xxRO */
        if ((mmu_ap & MMU_AP_RWRO) ||
            (mmu_ap & MMU_AP_RWRW) ||
            (mmu_ap & MMU_AP_RORO)) {
            return (1 == 1);
        } else {
            return 0;
        }
    }

    return retval;
}

int useracc(__user const void * addr, size_t len, int rw)
{
    if (!curproc)
        return 0;

    return useracc_proc(addr, len, curproc, rw);
}

int useracc_proc(__user const void * addr, size_t len, struct proc_info * proc,
                 int rw)
{
    /* TODO Currently we don't allow addr + len to span over multiple regions */

    struct buf * region;
    uintptr_t uaddr, start, end;

    if (addr == NULL)
        return 0;

    uaddr = (uintptr_t)addr;
    if (vm_find_reg(proc, uaddr, &region) == -1)
        return 0;

    start = region->b_mmu.vaddr;

    /*
     * Unfortunately sometimes the b_count is invalid.
     */
    if (unlikely(region->b_bcount == 0)) {
        /* TODO and this is probably wrong too */
        end = mmu_sizeof_region(&region->b_mmu);
    } else {
        end = region->b_bcount;
    }
    end += region->b_mmu.vaddr - 1;

    if ((rw & VM_PROT_WRITE) && (region->b_uflags & VM_PROT_COW)) {
        /* FIXME We should inform the called */
        KERROR(KERROR_WARN, "VMPROT_WRITE tested for COW region\n");
    }

    if (VM_ADDR_IS_IN_RANGE(uaddr, start, end))
        return test_ap_user(rw, region);
    return 0;
}

void vm_get_uapstring(char str[5], struct buf * bp)
{
    int uap = bp->b_uflags;

    str[0] = (uap & VM_PROT_READ) ?     'r' : '-';
    str[1] = (uap & VM_PROT_WRITE) ?    'w' : '-';
    str[2] = (uap & VM_PROT_EXECUTE) ?  'x' : '-';
    str[3] = (uap & VM_PROT_COW) ?      'c' : '-';
    str[4] = '\0';
}
