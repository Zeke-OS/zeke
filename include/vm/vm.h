/**
 *******************************************************************************
 * @file    vm.h
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

#ifndef _VM_VM_H
#define _VM_VM_H
#ifdef KERNEL_INTERNAL
#include <hal/mmu.h>
#include <klocks.h>

#define VM_PROT_READ    0x1 /*!< Read. */
#define VM_PROT_WRITE   0x2 /*!< Write. */
#define VM_PROT_EXECUTE 0x4 /*!< Execute. */
#define VM_PROT_COW     0x8 /*!< Copy-on-write. */

/**
 * VM management structure.
 * This struct type is used to manage memory regions in vm.
 * This struct is always a member of mmu_region struct.
 */
typedef struct vm_region {
    mmu_region_t mmu;
    int usr_rw; /*!< Actual user mode permissions on this data. Sometimes we
                 * want to set ap read-only to easily make copy-on-write or to
                 * pass control to MMU exception handler for some other reason
                 */

    /* Allocator specific data */
#if configDEBUG != 0
    unsigned int allocator_id; /*!< Optional allocator identifier. Allocator
                                * can use this to check that a given
                                * region is actually allocated with it. */
#endif
    void * allocator_data;      /*!< Optional allocator specific data. */
    struct vm_ops * vm_ops;
    int refcount;
    mtx_t lock;
} vm_region_t;

/**
 * VM operations.
 */
typedef struct vm_ops {
    /**
     * Increment region reference count.
     */
    void (*rref)(struct vm_region * this);

    /**
     * Pointer to a 1:1 region cloning function.
     * This function if set clones contents of the region to another
     * physical locatation.
     * @note Can be null.
     * @param cur_region    is a pointer to the current region.
     */
    struct vm_region * (*rclone)(struct vm_region * cur_region);

    /**
     * Free this region.
     * @note Can be null.
     * @param this is the current region.
     */
    void (*rfree)(struct vm_region * this);
} vm_ops_t;

/**
 * MM struct for processes.
 */
struct vm_mm_struct {
    mmu_pagetable_t mptable;    /*!< Process master page table. */
    /* TODO btree of page tables */
    vm_region_t * (*regions)[]; /*!< Memory regions of a process.
                                 *   [0] = code
                                 *   [1] = kstack
                                 *   [2] = stack
                                 *   [3] = heap/data
                                 *   [n] = allocs
                                 */
    int nr_regions;             /*!< Number of regions allocated. */
};

int copyin(const void * uaddr, void * kaddr, size_t len);
int copyout(const void * kaddr, void * uaddr, size_t len);
int copyinstr(const void * uaddr, void * kaddr, size_t len, size_t * done);

/* Not sure if these should be actually in vm_extern.h for BSD compatibility */
int kernacc(void * addr, int len, int rw);
int useracc(void * addr, int len, int rw);

#endif /* KERNEL_INTERNAL */
#endif /* _VM_VM_H */
