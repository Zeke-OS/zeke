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

#include <libkern.h>
#include <kstring.h>
#include <hal/mmu.h>
#include <ptmapper.h>
#include <kmalloc.h>
#include <kerror.h>
#include <errno.h>
#include <vm/vm.h>

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
    if (vaddr < MMU_PGSIZE_SECTION) {
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
    struct vm_pt * var;
    struct vm_pt * nxt;

    if (!RB_EMPTY(ptlist_head)) {
        for (var = RB_MIN(ptlist, ptlist_head); var != 0;
                var = nxt) {
            nxt = RB_NEXT(ptlist, ptlist_head, var);
            ptmapper_free(&(var->pt));
            kfree(var);
        }
    }
}

int vm_ptlist_clone(struct ptlist * new_head, mmu_pagetable_t * new_mpt,
                    struct ptlist * old_head)
{
    struct vm_pt * old_vpt;
    struct vm_pt * new_vpt;
    int count = 0;

    RB_INIT(new_head);

    if (RB_EMPTY(old_head))
        return 0;

    RB_FOREACH(old_vpt, ptlist, old_head) {
        /* TODO for some reason linkcount might be invalid! */
#if 0
        if (old_vpt->linkcount <= 0) {
            continue; /* Skip unused page tables; ie. page tables that are
                       * not referenced by any region. */
        }
#endif

        new_vpt = vm_pt_clone_attach(old_vpt, new_mpt);
        if (!new_vpt)
            return -ENOMEM;

        /* Insert new_vpt (L2 page table) to the new new process. */
        RB_INSERT(ptlist, new_head, new_vpt);

        count++; /* Increment vpt copied count. */
    }

    return count;
}

struct vm_pt * vm_pt_clone_attach(struct vm_pt * old_vpt, mmu_pagetable_t * mpt)
{
    struct vm_pt * new_vpt;

    KASSERT(old_vpt != NULL, "old_vpt should be set");

    new_vpt = kmalloc(sizeof(struct vm_pt));
    if (!new_vpt)
        return NULL;

    new_vpt->linkcount = 1;
    new_vpt->pt.vaddr = old_vpt->pt.vaddr;
    new_vpt->pt.master_pt_addr = mpt->pt_addr;
    new_vpt->pt.type = MMU_PTT_COARSE;
    new_vpt->pt.dom = old_vpt->pt.dom;

    /* Allocate the actual page table, this will also set pt_addr. */
    if (ptmapper_alloc(&(new_vpt->pt))) {
        kfree(new_vpt);
        return NULL;
    }

    mmu_ptcpy(&new_vpt->pt, &old_vpt->pt);
    mmu_attach_pagetable(&new_vpt->pt);

    return new_vpt;
}
