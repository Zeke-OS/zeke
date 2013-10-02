/**
 *******************************************************************************
 * @file    mmu.h
 * @author  Olli Vanhoja
 * @brief   MMU headers.
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

/** @addtogroup HAL
  * @{
  */

#pragma once
#ifndef MMU_H
#define MMU_H
#include <autoconf.h>
#if configMMU == 0
    #error MMU not enabled but header was included in some file.
#endif

#include <stdint.h>


/* Kernel memory map **********************************************************/
#define MMU_PT_BASE             0x00018000

/* TODO can we put these to memmap file? */

/** Base address of Page table region */
#define MMU_VADDR_MASTER_PT     MMU_PT_BASE
#define MMU_VADDR_KERNEL_START  0x00000000
//#define MMU_VADDR_KERNEL_END
#define MMU_VADDR_SHARED_START  0x00010000
//#define MMU_VADDR_SHARED_END
/**
 * Dynmem area starts
 * TODO check if this is ok?
 */
#define MMU_VADDR_DYNMEM_START  0x00020000
/**
 * Dynmem area end
 * TODO should match end of physical memory at least
 * (or higher if paging is allowed later)
 */
#define MMU_VADDR_DYNMEM_END    0x00050000

/**
 * Last static page table index.
 */
#define MMU_PT_LAST_SINDEX      1

/**
 * First dynamic page table address.
 */
#define MMU_PT_FIRST_DYNPT MMU_PT_ADDR(MMU_PT_LAST_SINDEX + 1)

/* Memory map macros */
/** Size of static L1 tables */
#define MMU_PT_L1TABLES (MMU_PTSZ_MASTER)

/**
 * A macro to calculate the address for statically allocated page table.
 *
 * Note: We assume that there is only one static master table and all other
 *       tables are equally sized coarse page tables.
 */
#define MMU_PT_ADDR(index) (MMU_PT_BASE + MMU_PT_L1TABLES + index * MMU_PTSZ_COARSE)
/* End of Kernel memory map ***************************************************/

/* Zeke Domains */
#define MMU_DOM_KERNEL  3 /*!< Kernel domain */
#define MMU_DOM_USER    3 /*!< User domain */


/* Page Table Types */
#define MMU_PTT_COARSE  MMU_PTE_COARSE  /*!< Coarse page table type. */
#define MMU_PTT_MASTER  MMU_PTE_SECTION /*!< Master page table type. */


/* Access Permissions control
 * NA = No Access, RO = Read Only, RW = Read/Write
 * +----+
 * |2 10|
 * +----+
 * |A A |
 * |P P |
 * |X   |
 * +----+
 */
#define MMU_AP_NANA    0x00 /*!< All accesses generate a permission fault */
#define MMU_AP_RWNA    0x01 /*!< Privileged access only */
#define MMU_AP_RWRO    0x02 /*!< Writes in User mode generate permission faults
                             * faults */
#define MMU_AP_RWRW    0x03 /*!< Full access */
#define MMU_AP_RONA    0x05 /*!< Privileged read-only and User no access */
#define MMU_AP_RORO    0x06 /*!< Privileged and User read-only */


/* Control bits
 * ============
 *
 * |31        |9       5|   4|  2|   1|  0|
 * +--------------------------------------+
 * | Not used | MEMTYPE | XN | - | nG | S |
 * +--------------------------------------+
 * S        = Shared
 * nG       = Determines if the translation is marked as global (0) or process
 *            specific (1).
 * XN       = Execute-Never, mark region not-executable.
 * MEMTYPE  = TEX C B
 *            987 6 5 b
 */

/* S */
#define MMU_CTRL_S_OFFSET       0
/** Shared memory */
#define MMU_CTRL_S              (0x1 << MMU_CTRL_S_OFFSET)

/* nG */
#define MMU_CTRL_NG_OFFSET      1
/** Not-Global, use ASID */
#define MMU_CTRL_NG             (0x1 << MMU_CTRL_NG_OFFSET)

/* XN */
#define MMU_CTRL_XN_OFFSET      4
/** Execute-Never */
#define MMU_CTRL_XN             (0x1 << MMU_CTRL_XN_OFFSET)

/* MEMTYPE */
#define MMU_CTRL_MEMTYPE_OFFSET 2
/** Strongly ordered, shared */
#define MMU_CTRL_MEMTYPE_SO     (0x0 << MMU_CTRL_MEMTYPE_OFFSET)
/** Non-shareable device */
#define MMU_CTRL_MEMTYPE_DEV    (0x8 << MMU_CTRL_MEMTYPE_OFFSET)
/** Shared device */
#define MMU_CTRL_MEMTYPE_SDEV   (0x1 << MMU_CTRL_MEMTYPE_OFFSET)
/** Write-Through, shareable */
#define MMU_CTRL_MEMTYPE_WT     (0x2 << MMU_CTRL_MEMTYPE_OFFSET)
/** Write-Back, shareable */
#define MMU_CTRL_MEMTYPE_WB     (0x3 << MMU_CTRL_MEMTYPE_OFFSET)

/* End of Control bits */


/**
 * Page Table Control Block - PTCB
 */
typedef struct {
    uint32_t vaddr;     /*!< identifies a starting address of a 1MB section of
                         * a virtual memory controlled by either a section
                         * entry or a L2 page table. */
    uint32_t pt_addr;   /*!< is the address where the page table is located in
                         * virtual memory. */
    uint32_t master_pt_addr; /*!< is the address of a parent master L1 page
                              * table. If the table is an L1 table, then the
                              * value is same as pt_addr. */
    uint32_t type;      /*!< identifies the type of the page table. */
    uint32_t dom;       /*!< is the domain of the page table. */
} mmu_pagetable_t;

/**
 * Region Control Block - RCB
 */
typedef struct {
    uint32_t vaddr;     /*!< is the virtual address that begins the region in
                         * virtual memory. */
    uint32_t num_pages; /*!< is the number of pages in the region. */
    uint32_t ap;        /*!< selects the region access permissions. */
    uint32_t control;   /*!< selects the cache, write buffer, execution and
                         * sharing (nG, S) attributes. */
    uint32_t paddr;     /*!< is the physical starting address of the region in
                         * virtual memory. */
    mmu_pagetable_t * pt; /*!< is a pointer to the page table in which
                           * the region resides. */
} mmu_region_t;


#if configARCH == __ARM6__ || __ARM6K__ /* ARM11 uses ARMv6 arch */
#include "arm11/arm11_mmu.h"
#else
    #error MMU for selected ARM profile/architecture is not supported
#endif

#endif /* MMU_H */

/**
  * @}
  */
