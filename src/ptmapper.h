/**
 *******************************************************************************
 * @file    ptmapper.h
 * @author  Olli Vanhoja
 * @brief   Page table mapper.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <hal/mmu.h>

/* Kernel memory map **********************************************************/
#define PTMAPPER_BASE           0x00100000

/* TODO can we put these to memmap file? */

/** Base address of Page table region */
#define MMU_VADDR_KERNEL_START  0x00000000
#define MMU_VADDR_KERNEL_END    0x0007FFFF
#define MMU_VADDR_SHARED_START  0x00080000
#define MMU_VADDR_SHARED_END    0x000FFFFF
/**
 * Dynmem area starts
 * TODO check if this is ok?
 */
#define MMU_VADDR_DYNMEM_START  0x00300000
/**
 * Dynmem area end
 * TODO should match end of physical memory at least
 * (or higher if paging is allowed later)
 * We possibly should use mmu_memsize somewhere,
 * but not here because this is used for some statical
 * allocations.
 */
#define MMU_VADDR_DYNMEM_END    0x00FFFFFF
/* End of Kernel memory map ***************************************************/

/* Page Table Region Macros ***************************************************/

/**
 * Page count by size of region.
 * @param size  Size of region.
 * @param psize Page size. 1 = 1 MB sections, 4096 = 4096 KB small pages
 * @return Region size in pages.
 */
#define MMU_PAGE_CNT_BY_SIZE(size, psize)           ((size)/(psize))

/**
 * Page count by address range.
 * @param begin Region start address.
 * @param end   Region end address.
 * @param psize Page size. 1 = 1 MB sections, 4096 = 4096 KB small pages
 * @return Region size in pages.
 */
#define MMU_PAGE_CNT_BY_RANGE(begin, end, psize)    (((end)-(begin)+1)/(psize))
/* End of Page Table Region Macros ********************************************/

int ptmapper_alloc(mmu_pagetable_t * pt);
