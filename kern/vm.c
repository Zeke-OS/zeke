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

/** @addtogroup LIBC
 * @{
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
#include <vm/vm.h>

extern mmu_region_t mmu_region_kernel;

static int test_ap_priv(uint32_t rw, uint32_t ap);
static int test_ap_user(uint32_t rw, uint32_t ap);


RB_GENERATE(ptlist, vm_pt, entry_, ptlist_compare);

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
int ptlist_compare(struct vm_pt * a, struct vm_pt * b)
{
    const ssize_t vaddr_a = (a) ? (ssize_t)(a->pt.vaddr) : 0;
    const ssize_t vaddr_b = (b) ? (ssize_t)(b->pt.vaddr) : 0;

    return (int)(vaddr_a - vaddr_b);
}

/**
 * Get a page table for given virtual address.
 * @param ptlist_heas is a structure containing all page tables.
 * @param mpt   is a master page table.
 * @param vaddr is the virtual address that will be mapped into a returned page
 *              table.
 * @return Returs a page table where vaddr can be mapped.
 */
struct vm_pt * ptlist_get_pt(struct ptlist * ptlist_head, mmu_pagetable_t * mpt, uintptr_t vaddr)
{
    struct vm_pt * vpt = 0;
    struct vm_pt filter = {
        .pt.vaddr = MMU_CPT_VADDR(vaddr)
    }; /* Used as a search filter */

#if configDEBUG >= KERROR_ERR
    if (!mpt)
        panic("mpt can't be null");
#endif

    /* Check if system page table is needed. */
    /* TODO I'm not sure if we have this one MB boundary as some fancy constant
     * somewhere but this should be quite stable thing though. */
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
        if(ptmapper_alloc(&(vpt->pt))) {
            kfree(vpt);
            return 0;
        }

        /* Insert vpt (L2 page table) to the new new process. */
        RB_INSERT(ptlist, ptlist_head, vpt);
        mmu_attach_pagetable(&(vpt->pt));
    }

    return vpt;
}

/**
 * Free ptlist and its page tables.
 */
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
 * @remark Compatible with BSD.
 * @param[in]   uaddr is the source address.
 * @param[out]  kaddr is the target address.
 * @param       len  is the length of source.
 * @return 0 if succeeded; otherwise EFAULT.
 */
int copyin(const void * uaddr, void * kaddr, size_t len)
{
    struct vm_pt * vpt;
    void * phys_uaddr;

    /* By now we believe existence of the requested address is already asserted
     * and we can just do the page table translation and copy data. */

    vpt = ptlist_get_pt(
            &(curproc->mm.ptlist_head),
            &(curproc->mm.mpt),
            (uintptr_t)uaddr);

    if (!vpt)
        panic("Can't copyin()");

    phys_uaddr = mmu_translate_vaddr(&(vpt->pt), (uintptr_t)uaddr);
    if (!phys_uaddr) {
        return EFAULT;
    }

    memcpy(kaddr, phys_uaddr, len);
    return 0;
}

/**
 * Copy data from kernel-space to user-space.
 * Copy len bytes of data from the kernel-space address kaddr to the user-space
 * address uaddr.
 * @remark Compatible with BSD.
 * @param[in]   kaddr is the source address.
 * @param[out]  uaddr is the target address.
 * @param       len is the length of source.
 * @return 0 if succeeded; otherwise EFAULT.
 */
int copyout(const void * kaddr, void * uaddr, size_t len)
{
    /* TODO Handle possible cow flag? */
    struct vm_pt * vpt;
    void * phys_uaddr;

    vpt = ptlist_get_pt(
            &(curproc->mm.ptlist_head),
            &(curproc->mm.mpt),
            (uintptr_t)uaddr);

    if (!vpt)
        panic("Can't copyout()");

    phys_uaddr = mmu_translate_vaddr(&(vpt->pt), (uintptr_t)uaddr);
    if (!phys_uaddr)
        return EFAULT;

    memcpy(phys_uaddr, kaddr, len);
    return 0;
}

/**
 * Copy a string from user-space to kernel-space.
 * Copy a NUL-terminated string, at most len bytes long, from user-space address
 * uaddr to kernel-space address kaddr.
 * @remark Compatible with BSD.
 * @param[in]   uaddr is the source address.
 * @param[out]  kaddr is the target address.
 * @param       len is the length of string in uaddr.
 * @param[out]  done is the number of bytes actually copied, including the
 *                   terminating NUL, if done is non-NULL.
 * @return  0 if succeeded; or ENAMETOOLONG if the string is longer than len
 *          bytes.
 */
int copyinstr(const void * uaddr, void * kaddr, size_t len, size_t * done)
{
    size_t retval_cpy;
    int retval = 0;

    /* TODO Translation */
    retval_cpy = strlcpy(kaddr, uaddr, len);
    if (retval_cpy >= len) {
        if (done != 0)
            *done = len;
        retval = ENAMETOOLONG;
    } else if (done != 0) {
        *done = retval_cpy;
    }

    return retval;
}

/**
 * @}
 */

/**
 * Update usr access permissions based on region->usr_rw.
 * @param region is the region to be updated.
 */
void vm_updateusr_ap(struct vm_region * region)
{
    int usr_rw;
    unsigned ap;

    mtx_spinlock(&(region->lock));
    usr_rw = region->usr_rw;
    ap = region->mmu.ap;

#define _COWRD (VM_PROT_COW | VM_PROT_READ)
    if ((usr_rw & _COWRD) == _COWRD) {
        region->mmu.ap = MMU_AP_RORO;
    } else if (usr_rw & VM_PROT_WRITE) {
        region->mmu.ap = MMU_AP_RWRW;
    } else if (usr_rw & VM_PROT_READ) {
        switch (ap) {
            case MMU_AP_NANA:
            case MMU_AP_RONA:
                region->mmu.ap = MMU_AP_RORO;
                break;
            case MMU_AP_RWNA:
                region->mmu.ap = MMU_AP_RWRO;
                break;
            case MMU_AP_RWRO:
                break;
            case MMU_AP_RWRW:
                region->mmu.ap = MMU_AP_RWRO;
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
                region->mmu.ap = MMU_AP_RWNA;
                break;
            case MMU_AP_RORO:
                region->mmu.ap = MMU_AP_RONA;
                break;
        }
    }
#undef _COWRD

    mtx_unlock(&(region->lock));
}

/**
 * Map a VM region with the given page table.
 * @note vm_region->mmu.pt is not respected and is considered as invalid.
 * @param vm_region is a vm region.
 * @param vm_pt is a vm page table.
 * @return Zero if succeed; non-zero error code otherwise.
 */
int vm_map_region(vm_region_t * vm_region, struct vm_pt * pt)
{
    mmu_region_t mmu_region;

#if configDEBUG >= KERROR_ERR
    if (vm_region == 0)
        panic("vm_region can't be null");
#endif

    vm_updateusr_ap(vm_region);
    mtx_spinlock(&(vm_region->lock));

    mmu_region = vm_region->mmu; /* Make a copy of mmu region struct */
    mmu_region.pt = &(pt->pt);

    mtx_unlock(&(vm_region->lock));

    return mmu_map_region(&mmu_region);
}

/**
 * Check kernel space memory region for accessibility.
 * Check whether operations of the type specified in rw are permitted in the
 * range of virtual addresses given by addr and len.
 * @todo Not implemented properly!
 * @remark Compatible with BSD.
 * @return  Boolean true if the type of access specified by rw is permitted;
 *          Otherwise boolean false.
 */
int kernacc(void * addr, int len, int rw)
{
    size_t reg_start, reg_size;
    uint32_t ap;

    reg_start = mmu_region_kernel.vaddr;
    reg_size = mmu_sizeof_region(&mmu_region_kernel);
    if (((size_t)addr >= reg_start) && ((size_t)addr <= reg_start + reg_size))
        return (1 == 1);

    /* TODO Check other static regions as well */

    if ((ap = dynmem_acc(addr, len))) {
        if (test_ap_priv(rw, ap))
            return (1 == 1);
        //else
        //    goto out;
    }

    KERROR(KERROR_WARN, "Can't fully verify access to address in kernacc()");

    return (1 == 1); /* TODO */
//out:
//    return 0;
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
        if(ap & 0x8)
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

/**
 * Check user space memeory region for accessibility.
 * Check whether operations of the type specified in rw are permitted in the
 * range of virtual addresses given by addr and len.
 * This function considers addr to represent an user space address. The process
 * context to use for this operation is taken from the global variable curproc.
 * @todo Not implemented properly!
 * @remark Compatible with BSD.
 * @return  Boolean true if the type of access specified by rw is permitted;
 *          Otherwise boolean false.
 */
int useracc(void * addr, int len, int rw)
{
    /* TODO We may wan't to handle cow here */
    /* TODO */
    //if (curproc == 0)
    //    return 0;

    return (1 == 1);
}

/**
 * Test for user mode access permissions.
 *
 * AP format for this function:
 * 3  2    0
 * +--+----+
 * |XN| AP |
 * +--+----+
 */
static int test_ap_user(uint32_t rw, uint32_t ap)
{
    if (rw & VM_PROT_EXECUTE) {
        if(ap & 0x8)
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

/**
 * @}
 */
