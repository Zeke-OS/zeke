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
#include <llist.h>
#include <sys/tree.h>
#include <hal/mmu.h>
#include <klocks.h>
#include <fs/fs.h>

#define VM_PROT_READ    0x1 /*!< Read. */
#define VM_PROT_WRITE   0x2 /*!< Write. */
#define VM_PROT_EXECUTE 0x4 /*!< Execute. */
#define VM_PROT_COW     0x8 /*!< Copy-on-write. */

struct proc_info;

/**
 * VM page table structure.
 */
struct vm_pt {
    RB_ENTRY(vm_pt) entry_;
    mmu_pagetable_t pt;
    int linkcount; /*!< Links to this page table */
};

/* struct ptlist */
RB_HEAD(ptlist, vm_pt);

/**
 * MM struct for processes.
 */
struct vm_mm_struct {
    mmu_pagetable_t mpt;        /*!< Process master page table. */
    mmu_pagetable_t * curr_mpt; /*!< Current master page table. */
    /** RB tree of page tables */
    struct ptlist ptlist_head;
#define MM_CODE_REGION  0
#define MM_STACK_REGION 1
#define MM_HEAP_REGION  2
    struct buf * (*regions)[]; /*!< Memory regions of a process.
                                 *   [0] = code         RORO
                                 *   [1] = stack        RWRW
                                 *   [2] = heap/data    RWRW
                                 *   [n] = allocs
                                 */
    int nr_regions;             /*!< Number of regions allocated. */
};

/**
 * Compare vmp_pt rb tree nodes.
 * Compares virtual addresses of two page tables.
 * @param a is the left node.
 * @param b is the right node.
 * @return  If the first argument is smaller than the second, the function
 *          returns a value smaller than zero;
 *          If they are equal, the  function returns zero;
 *          Otherwise, value greater than zero is returned.
 */
int ptlist_compare(struct vm_pt * a, struct vm_pt * b);
RB_PROTOTYPE(ptlist, vm_pt, entry_, ptlist_compare);

/**
 * Get a page table for a given virtual address.
 * @param ptlist_head   is a structure containing all page tables.
 * @param mpt           is a master page table.
 * @param vaddr         is the virtual address that will be mapped into
 *                      a returned page table.
 * @return Returs a page table where vaddr can be mapped.
 */
struct vm_pt * ptlist_get_pt(struct ptlist * ptlist_head, mmu_pagetable_t * mpt,
        uintptr_t vaddr);

/**
 * Free ptlist and its page tables.
 */
void ptlist_free(struct ptlist * ptlist_head);

/** @addtogroup copy copyin, copyout, copyinstr
 * Kernel copy functions.
 *
 * The copy functions are designed to copy contiguous data from one address
 * to another from user-space to kernel-space and vice-versa.
 * @{
 */

/**
 * Copy data from user-space to kernel-space.
 * Copy len bytes of data from the user-space address uaddr to the kernel-space
 * address kaddr.
 * @param[in]   uaddr is the source address.
 * @param[out]  kaddr is the target address.
 * @param       len  is the length of source.
 * @return 0 if succeeded; otherwise -EFAULT.
 */
int copyin(const void * uaddr, void * kaddr, size_t len);

/**
 * Copy data from kernel-space to user-space.
 * Copy len bytes of data from the kernel-space address kaddr to the user-space
 * address uaddr.
 * @param[in]   kaddr is the source address.
 * @param[out]  uaddr is the target address.
 * @param       len is the length of source.
 * @return 0 if succeeded; otherwise -EFAULT.
 */
int copyout(const void * kaddr, void * uaddr, size_t len);

/**
 * Copy a string from user-space to kernel-space.
 * Copy a NUL-terminated string, at most len bytes long, from user-space address
 * uaddr to kernel-space address kaddr.
 * @param[in]   uaddr is the source address.
 * @param[out]  kaddr is the target address.
 * @param       len is the length of string in uaddr.
 * @param[out]  done is the number of bytes actually copied, including the
 *                   terminating NUL, if done is non-NULL.
 * @return  0 if succeeded; or -ENAMETOOLONG if the string is longer than len
 *          bytes; or any of the return values defined for copyin().
 */
int copyinstr(const void * uaddr, void * kaddr, size_t len, size_t * done);

int copyin_proc(struct proc_info * proc, const void * uaddr, void * kaddr,
        size_t len);
int copyout_proc(struct proc_info * proc, const void * kaddr, void * uaddr,
        size_t len);

/**
 * @}
 */

/**
 * Update usr access permissions based on b_uflags.
 * @param region is the region to be updated.
 */
void vm_updateusr_ap(struct buf * region);

int vm_add_region(struct vm_mm_struct * mm, struct buf * region);

int vm_replace_region(struct vm_mm_struct * mm, struct buf * region,
                      int region_nr);

/**
 * Map a VM region with the given page table.
 * @note buf->b_mmu.pt is not respected and is considered as invalid.
 * @param region is a vm region buffer.
 * @param vm_pt is a vm page table.
 * @return Zero if succeed; non-zero error code otherwise.
 */
int vm_map_region(struct buf * region, struct vm_pt * pt);

/**
 * Map a VM region to given a process pointed by proc.
 * @param proc is the process struct.
 * @param region is a vm region buffer.
 */
int vm_mapproc_region(struct proc_info * proc, struct buf * region);

/**
 * Check kernel space memory region for accessibility.
 * Check whether operations of the type specified in rw are permitted in the
 * range of virtual addresses given by addr and len.
 * @todo Not implemented properly!
 * @remark Compatible with BSD.
 * @return  Boolean true if the type of access specified by rw is permitted;
 *          Otherwise boolean false.
 */
int kernacc(const void * addr, int len, int rw);

/**
 * Test for priv mode access permissions.
 *
 * AP format for this function:
 * 3  2    0
 * +--+----+
 * |XN| AP |
 * +--+----+
 */
int useracc(const void * addr, int len, int rw);

#endif /* KERNEL_INTERNAL */
#endif /* _VM_VM_H */
