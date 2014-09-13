/**
 *******************************************************************************
 * @file    ptmapper.h
 * @author  Olli Vanhoja
 * @brief   Page table mapper.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup ptmapper
 * @{
 */

#pragma once
#ifndef PTMAPPER_H
#define PTMAPPER_H

#include <autoconf.h>
#include <hal/mmu.h>
#include <vm/vm.h>

/* Kernel memory map **********************************************************/
/* page table area */
#ifdef configPT_AREA_START
#define PTMAPPER_PT_START       configPT_AREA_START
#define PTMAPPER_PT_END         configPT_AREA_END
#endif

#ifdef configKSTACK_START
#define MMU_VADDR_KSTACK_START  configKSTACK_START
#define MMU_VADDR_KSTACK_END    configKSTACK_END
#endif

/** TKSTACK is a thread local kernel mode stack and its NOT mapped 1:1 unlike
 * other regions defined here. */
#ifdef configTKSTACK_START
#define MMU_VADDR_TKSTACK_START configTKSTACK_START
#define MMU_VADDR_TKSTACK_END   configTKSTACK_END
#endif

#ifdef configKERNEL_START
#define MMU_VADDR_KERNEL_START  configKERNEL_START
#define MMU_VADDR_KERNEL_END    configKERNEL_END
#endif
#if 0
#define MMU_VADDR_SHARED_START  0x00080000
#define MMU_VADDR_SHARED_END    0x000FFFFF /* 1M; End of system page table */
#endif
/**
 * Begining of dynmem area.
 */
#ifdef configDYNMEM_START
#define MMU_VADDR_DYNMEM_START  configDYNMEM_START
/**
 * End of dynmem area.
 * TODO should match end of physical memory at least
 * (or higher if paging is allowed later)
 * We possibly should use mmu_memsize somewhere,
 * but not here because this is used for some statical
 * allocations.
 */
#define MMU_VADDR_DYNMEM_END    configDYNMEM_END
#endif

/* TODO These shouldn't be here actually */
#ifdef configBCM2835
#define MMU_VADDR_RPIHW_START   0x20000000
#define MMU_VADDR_RPIHW_END     0x20FFFFFF
#endif

/* Kernel dynamic sections */
#ifdef configKSECT_START
#define MMU_VADDR_KSECT_START   configKSECT_START
#define MMU_VADDR_KSECT_END     configKSECT_END
#endif
/* End of Kernel memory map ***************************************************/

/* Page Table Region Macros ***************************************************/

/**
 * Page count by size of region.
 * @param size  Size of region.
 * @param psize Page size. 1024*1024 = 1 MB sections, 4096 = 4 KB small pages
 * @return Region size in pages.
 */
#define MMU_PAGE_CNT_BY_SIZE(size, psize)           ((size)/(psize))

/**
 * Page count by address range.
 * @param begin Region start address.
 * @param end   Region end address.
 * @param psize Page size. 1024*1024 = 1 MB sections, 4096 = 4 KB small pages
 * @return Region size in pages.
 */
#define MMU_PAGE_CNT_BY_RANGE(begin, end, psize)    (((end)-(begin)+1)/(psize))
/* End of Page Table Region Macros ********************************************/

extern mmu_pagetable_t mmu_pagetable_master;
extern mmu_pagetable_t mmu_pagetable_system;
struct vm_pt vm_pagetable_system;

extern const mmu_region_t mmu_region_kstack;
extern mmu_region_t mmu_region_kernel;
extern mmu_region_t mmu_region_kdata;

/**
 * Allocate memory for a page table.
 * Allocate memory for a page table from the page table region. This function
 * will not activate the page table or do anything besides updating necessary
 * varibles in pt and reserve a block of memory from the area.
 * @note master_pt_addr will be only set for a page table struct if allocated
 * page table is a master page table.
 * @param pt    is the page table struct without page table address pt_addr.
 * @return  Returns zero if succeed; Otherwise value other than zero indicating
 *          that the page table allocation has failed.
 */
int ptmapper_alloc(mmu_pagetable_t * pt);

/**
 * Free page table.
 * Frees a page table that has been previously allocated with ptmapper_alloc.
 * @note    Page table pt should be detached properly before calling this
 *          function and it is usually good idea to unmap any regions still
 *          mapped with the page table before removing it completely.
 * @param pt is the page table to be freed.
 */
void ptmapper_free(mmu_pagetable_t * pt);

#endif /* PTMAPPER_H */

/**
 * @}
 */
