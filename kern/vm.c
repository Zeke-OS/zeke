/**
 *******************************************************************************
 * @file    vm.c
 * @author  Olli Vanhoja
 * @brief   VM functions.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

extern mmu_region_t mmu_region_kernel;

static int test_ap_priv(uint32_t rw, uint32_t ap);
static int test_ap_user(uint32_t rw, uint32_t ap);


RB_GENERATE(ptlist, vm_pt, entry_, ptlist_compare);

int ptlist_compare(struct vm_pt * a, struct vm_pt * b)
{
    const ptrdiff_t vaddr_a = (a) ? (ptrdiff_t)(a->pt.vaddr) : 0;
    const ptrdiff_t vaddr_b = (b) ? (ptrdiff_t)(b->pt.vaddr) : 0;

    return (int)(vaddr_a - vaddr_b);
}

struct vm_pt * ptlist_get_pt(struct ptlist * ptlist_head, mmu_pagetable_t * mpt,
        uintptr_t vaddr)
{
    struct vm_pt * vpt = 0;
    struct vm_pt filter = {
        .pt.vaddr = MMU_CPT_VADDR(vaddr)
    }; /* Used as a search filter */

    KASSERT(mpt, "mpt can't be null");

    /*
     * Check if system page table is needed.
     *
     * TODO I'm not quite sure if we have this one MB boundary as
     * a fancy constant somewhere but this should be ok.
     */
    if (vaddr <= 0x000FFFFF) {
        return &vm_pagetable_system;
    }

    /* Look for existing page table. */
    if (!RB_EMPTY(ptlist_head)) {
        vpt = RB_FIND(ptlist, ptlist_head, &filter);
    }
    if (!vpt) { /* Create a new pt if a sufficient pt not found. */
        vpt = kcalloc(1, sizeof(struct vm_pt));
        if (!vpt) {
            return 0;
        }

        vpt->pt.vaddr = filter.pt.vaddr;
        vpt->pt.master_pt_addr = mpt->pt_addr;
        vpt->pt.type = MMU_PTT_COARSE;
        vpt->pt.dom = MMU_DOM_USER;

        /* Allocate the actual page table, this will also set pt_addr. */
        if (ptmapper_alloc(&(vpt->pt))) {
            kfree(vpt);
            return 0;
        }

        /* Insert vpt (L2 page table) to the new new process. */
        RB_INSERT(ptlist, ptlist_head, vpt);
        mmu_attach_pagetable(&(vpt->pt));
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

    if (!useracc(uaddr, len, VM_PROT_READ)) {
        return -EFAULT;
    }

    vpt = ptlist_get_pt(
            &(curproc->mm.ptlist_head),
            &(curproc->mm.mpt),
            (uintptr_t)uaddr);

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

    if (!useracc(uaddr, len, VM_PROT_WRITE)) {
        return -EFAULT;
    }

    vpt = ptlist_get_pt(
            &(proc->mm.ptlist_head),
            &(proc->mm.mpt),
            (uintptr_t)uaddr);

    if (!vpt)
        panic("Can't copyout()");

    phys_uaddr = mmu_translate_vaddr(&(vpt->pt), (uintptr_t)uaddr);
    if (!phys_uaddr)
        return -EFAULT;

    memcpy(phys_uaddr, kaddr, len);
    return 0;
}

int copyinstr(const void * uaddr, void * kaddr, size_t len, size_t * done)
{
    size_t slen;
    int err;

    /*
     * There is two things user may try to use this functions for, copying
     * a string thats length is already known and copying a string of unknown
     * length to a buffer. In a case of unknown string we may fail to copy if
     * the string is too close to the page bound and add + len is out of bounds,
     * to solve this but to optimize of performance we do the following funny
     * looking loop-copyin.
     */
    while ((err = copyin(uaddr, kaddr, len))) {
        if (len == 1)
            break;
        len--;
    }
    if (err)
        return err;

    slen = strlenn(kaddr, len);
    if (done) {
        *done = slen + 1;
    }

    if (((char *)kaddr)[slen] != '\0')
        return -ENAMETOOLONG;

    return 0;
}

void vm_updateusr_ap(struct buf * region)
{
    int usr_rw;
    unsigned ap;

    mtx_lock(&(region->lock));
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

    mtx_unlock(&(region->lock));
}

int vm_add_region(struct vm_mm_struct * mm, struct buf * region)
{
    struct buf * (*new_regions)[];

    new_regions = krealloc(mm->regions,
            (mm->nr_regions + 1) * sizeof(struct buf *));
    if (!new_regions)
        return -ENOMEM;

    (*mm->regions)[mm->nr_regions] = region;
    mm->nr_regions = mm->nr_regions + 1;

    return 0;
}

int vm_map_region(struct buf * region, struct vm_pt * pt)
{
    mmu_region_t mmu_region;

    KASSERT(region, "region can't be null\n");

    vm_updateusr_ap(region);
    mtx_lock(&(region->lock));

    mmu_region = region->b_mmu; /* Make a copy of mmu region struct */
    mmu_region.pt = &(pt->pt);

    mtx_unlock(&(region->lock));

    return mmu_map_region(&mmu_region);
}

int vm_addrmap_region(struct proc_info * proc, struct buf * region,
        uintptr_t vaddr)
{
    struct vm_pt * vpt;

    vpt = ptlist_get_pt(&proc->mm.ptlist_head, &proc->mm.mpt, vaddr);
    if (!vpt)
        return -ENOMEM;

    region->b_mmu.vaddr = vaddr;

    return vm_map_region(region, vpt);
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
    KERROR(KERROR_WARN, "Can't fully verify access to address in kernacc()\n");
#endif

    return (1 == 1);
}

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

int useracc(const void * addr, int len, int rw)
{
    /* TODO We may wan't to handle cow here */
    /* TODO Implementation */
#if 0
    if (!curproc)
        return 0;
#endif

    return (1 == 1);
}

static int test_ap_user(uint32_t rw, uint32_t ap)
{
    if (rw & VM_PROT_EXECUTE) {
        if (ap & 0x8)
            return 0; /* XN bit set. */
    }
    ap &= ~0x8; /* Discard XN bit. */

    if (rw & VM_PROT_WRITE) { /* Test for xxRW */
        switch (ap) {
        case MMU_AP_RWRW:
            return (1 == 1);
        default:
            return 0; /* No write access. */
        }
    } else if (rw & VM_PROT_READ) { /* Test for xxRO */
        switch (ap) {
        case MMU_AP_RWRW: /* I guess rw is ok too. */
        case MMU_AP_RWRO:
        case MMU_AP_RORO:
            return (1 == 1);
        default:
            return 0; /* No read access. */
        }
    }

    return 0;
}
