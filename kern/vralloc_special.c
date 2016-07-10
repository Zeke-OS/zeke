/**
 *******************************************************************************
 * @file    vralloc_special.c
 * @author  Olli Vanhoja
 * @brief   Virtual Region Allocator.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <ptmapper.h>
#include <errno.h>
#include <kerror.h>
#include <proc.h>
#include <vm/vm.h>
#include <buf.h>

/* TODO We may want to use bitmaps */

mtx_t l_ksect_next;
static uintptr_t ksect_next = configKSECT_START;

static uintptr_t get_ksect_addr(size_t region_size)
{
    uintptr_t retval;

    if (l_ksect_next.mtx_type == MTX_TYPE_UNDEF)
        mtx_init(&l_ksect_next, MTX_TYPE_SPIN, 0);

    mtx_lock(&l_ksect_next);

    if (region_size >= MMU_PGSIZE_SECTION)
        retval = memalign_size(ksect_next, MMU_PGSIZE_SECTION);
    else
        retval = memalign_size(ksect_next, MMU_PGSIZE_COARSE);

    if (retval > configKSECT_END)
        return 0;

    ksect_next = retval + region_size;

    mtx_unlock(&l_ksect_next);

    return retval;
}

struct buf * geteblk_special(size_t size, uint32_t control)
{
    struct proc_info * proc = proc_ref(0);
    const uintptr_t kvaddr = get_ksect_addr(size);
    struct buf * buf;
    int err;

    KASSERT(proc, "Can't get the PCB of pid 0");
    proc_unref(proc);

    if (kvaddr == 0) {
        KERROR_DBG("Returned kvaddr is NULL\n");
        return NULL;
    }

    buf = vm_newsect(kvaddr, size, VM_PROT_READ | VM_PROT_WRITE);
    if (!buf) {
        KERROR_DBG("vm_newsect() failed\n");
        return NULL;
    }

    buf->b_mmu.control = control;

    err = vm_insert_region(proc, buf, VM_INSOP_MAP_REG);
    if (err < 0) {
        panic("Mapping a kernel special buffer failed");
    }

    buf->b_data = buf->b_mmu.vaddr; /* Should be same as kvaddr */

    return buf;
}
