/**
 *******************************************************************************
 * @file    vm_vpt.c
 * @author  Olli Vanhoja
 * @brief   VM functions.
 * @section LICENSE
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
#include <hal/mmu.h>
#include <kerror.h>
#include <kmalloc.h>
#include <kmem.h>
#include <kstring.h>
#include <libkern.h>
#include <ptmapper.h>
#include <vm/vm.h>

RB_GENERATE(ptlist, vm_pt, entry_, ptlist_compare);

static inline size_t bsize2nr_tables(size_t bsize)
{
    return memalign_size(bsize, MMU_PGSIZE_SECTION) / MMU_PGSIZE_SECTION;
}

static inline size_t nr_tables2bsize(size_t nr_tables)
{
    return nr_tables * MMU_PGSIZE_SECTION;
}

int ptlist_compare(struct vm_pt * a, struct vm_pt * b)
{
    uintptr_t a_start;
    uintptr_t b_start;

    KASSERT(a && b, "vm_pts are set");

    a_start = a->pt.vaddr;
    b_start = b->pt.vaddr;

    /*
     * In case we are searching with a filter we are interested whether
     * the filter vm_pt is inside the range of an entry in the tree.
     */
    if (a->flags & VM_PT_FLAG_FILTER) {
        uintptr_t b_end = b->pt.vaddr + mmu_sizeof_pt_img(&b->pt) - 1;

        if (VM_ADDR_IS_IN_RANGE(a_start, b_start, b_end))
            return 0;
    } else if (b->flags & VM_PT_FLAG_FILTER) {
        uintptr_t a_end = a->pt.vaddr +  mmu_sizeof_pt_img(&a->pt) - 1;

        if (VM_ADDR_IS_IN_RANGE(b_start, a_start, a_end))
            return 0;
    }

    return (int)((ptrdiff_t)a_start - (ptrdiff_t)b_start);
}

static struct vm_pt * vm_pt_alloc(size_t nr_tables)
{
    struct vm_pt * vpt = kzalloc(sizeof(struct vm_pt));
    if (!vpt)
        return NULL;

    vpt->pt.nr_tables = nr_tables;
    vpt->pt.pt_type = MMU_PTT_COARSE;
    vpt->pt.pt_dom = MMU_DOM_USER;

    /* Allocate the actual page table, this will also set pt_addr. */
    if (ptmapper_alloc(&vpt->pt)) {
        kfree(vpt);
        return NULL;
    }

    return vpt;
}

static void vm_pt_free(struct vm_pt * vpt)
{
    ptmapper_free(&vpt->pt);
    kfree(vpt);
}

struct vm_pt * ptlist_get_pt(struct vm_mm_struct * mm, uintptr_t vaddr,
                             size_t minsize, int flags)
{
    struct ptlist * const ptlist_head = &mm->ptlist_head;
    struct vm_pt * vpt;
    size_t nr_tables = bsize2nr_tables(minsize);

    /*
     * Check if the requested page table is actually the system pagetable.
     */
    if (vaddr >= vm_pagetable_system.pt.vaddr &&
        vaddr < (vm_pagetable_system.pt.vaddr +
                 mmu_sizeof_pt_img(&vm_pagetable_system.pt))) {
        return &vm_pagetable_system;
    }

    /* Look for an existing page table. */
    if (!RB_EMPTY(ptlist_head)) {
        struct vm_pt filter = {
            .pt.vaddr = MMU_CPT_VADDR(vaddr),
            .pt.nr_tables = 1,
            .flags = VM_PT_FLAG_FILTER,
        };

        vpt = RB_FIND(ptlist, ptlist_head, &filter);
    } else {
        vpt = NULL;
    }
    if (vpt) {
        uintptr_t vpt_start = vpt->pt.vaddr;
        uintptr_t vpt_end   = vpt_start + mmu_sizeof_pt_img(&vpt->pt) - 1;

        if (!VM_ADDR_IS_IN_RANGE(vaddr, vpt_start, vpt_end)) {
            KERROR(KERROR_ERR, "vaddr (%u) not in vpt (%p) %u - %u\n",
                   (unsigned)vaddr, vpt,
                   (unsigned)vpt_start, (unsigned)vpt_end);
            return NULL;
        }
        if (vpt->pt.nr_tables < nr_tables) {
            /*
             * FIXME If the returned vpt is too small then we need to do
             *       something.
             */
            KERROR(KERROR_ERR, "Too small vpt act=%d < minsize=%d\n",
                   mmu_sizeof_pt_img(&vpt->pt), minsize);
            return NULL;
        }
    } else if ((flags & VM_PT_CREAT) == VM_PT_CREAT) {
        /* Create a new pt if a sufficient pt was not found. */
        mmu_pagetable_t * const mpt = &mm->mpt;
        int err;

        KASSERT(mpt, "mpt can't be null");

        vpt = vm_pt_alloc(nr_tables);
        if (!vpt)
            return NULL;

        vpt->pt.vaddr = MMU_CPT_VADDR(vaddr);
        vpt->pt.master_pt_addr = mpt->pt_addr;

        /* Insert vpt (L2 page table) to the process. */
        RB_INSERT(ptlist, ptlist_head, vpt);
        err = mmu_attach_pagetable(&vpt->pt);
        if (err) {
            RB_REMOVE(ptlist, ptlist_head, vpt);
            vm_pt_free(vpt);
            KERROR(KERROR_ERR, "Can't attach a new pt to a ptlist (%p)\n",
                   ptlist_head);

            return NULL;
        }
    }

    return vpt;
}

void ptlist_free(struct ptlist * ptlist_head)
{
    struct vm_pt * var;
    struct vm_pt * nxt;

    if (RB_EMPTY(ptlist_head))
        return;

    for (var = RB_MIN(ptlist, ptlist_head); var != 0;
            var = nxt) {
        nxt = RB_NEXT(ptlist, ptlist_head, var);
        vm_pt_free(var);
    }
}

static struct vm_pt * vm_pt_clone_attach(struct vm_pt * old_vpt,
                                         mmu_pagetable_t * mpt)
{
    struct vm_pt * new_vpt;

    KASSERT(old_vpt != NULL, "old_vpt should be set");

    new_vpt = vm_pt_alloc(old_vpt->pt.nr_tables);
    if (!new_vpt)
        return NULL;

    new_vpt->pt.vaddr = old_vpt->pt.vaddr;
    new_vpt->pt.nr_tables = old_vpt->pt.nr_tables;
    new_vpt->pt.master_pt_addr = mpt->pt_addr;
    new_vpt->pt.pt_dom = old_vpt->pt.pt_dom;

    mmu_ptcpy(&new_vpt->pt, &old_vpt->pt);
    mmu_attach_pagetable(&new_vpt->pt);

    return new_vpt;
}

int vm_ptlist_clone(struct ptlist * new_head, mmu_pagetable_t * new_mpt,
                    struct ptlist * old_head)
{
    struct vm_pt * old_vpt;
    int count = 0;

    RB_INIT(new_head);

    if (RB_EMPTY(old_head))
        return 0;

    RB_FOREACH(old_vpt, ptlist, old_head) {
        struct vm_pt * new_vpt;

        new_vpt = vm_pt_clone_attach(old_vpt, new_mpt);
        if (!new_vpt)
            return -ENOMEM;

        /* Insert new_vpt (L2 page table) to new_head. */
        RB_INSERT(ptlist, new_head, new_vpt);

        count++; /* Increment vpt copied count. */
    }

    return count;
}
