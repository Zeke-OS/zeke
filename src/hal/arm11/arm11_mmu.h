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

/** @addtogroup ARM11
  * @{
  */

#pragma once
#ifndef ARM11_MMU_H
#define ARM11_MMU_H

#include "../mmu.h"

/**
 * Number of section entries in L1 page table.
 * Page size of one section is 1MB
 */
#define MMU_SECTIONS    4096

/**
 * Size of translation table pointed by TTBR0.
 *
 * N    VMS     pt size Entries
 * ----------------------------
 * 0    4GB     16KB    4096    (ARMv5 behaviour)
 * 1    2GB     8KB     2048
 * 2    1GB     4KB     1024
 * 3    512MB   2KB     512
 * 4    256MB   1KB     256
 * 5    128MB   512B    128
 * 6    64MB    256B    64
 * 7    32MB    128B    32
 */
#define MMU_TTBCR_N     7

/* L1 Page Table Entry Types */
#define MMU_L1_FAULT    0 /*!< MMU fault. */
#define MMU_L1_COARSE   1 /*!< Coarse page table. */
#define MMU_L1_MASTER   2 /*!< Section entry. */
#define MMU_L1_FINE     3 /*!< Fine page table. */

/* L2 Page Table Entry Types */
#define MMU_L2_FAULT    0
#define MMU_L2_LARGE    1 /*<! Large 64KB page frame */
#define MMU_L2_SMALL    2 /*<! Small 4KB page frame */
/*#define MMU_L2_TINY     3*/

/* TODO Use types */
/* TODO Not Global, Shared & XN bits */

/**
 * Page Table Control Block - PTCB
 */
typedef struct {
    uint32_t vAddr;     /*!< identifies a starting address of a 1MB section of
                          a virtual memory controlled by either a section
                          entry or a L2 page table. */
    uint32_t ptAddr;    /*!< is the address where the page table is located in
                            virtual memory. */
    uint32_t masterPtAddr;  /*!< is the address of a parent master L1 page
                              table. If the table is an L1 table, then the
                              value is same as ptAddr. */
    uint32_t type;      /*!< identifies the type of the page table. */
    uint32_t dom;       /*!< identifies the domain assigned to the page table
                          entry. */
} mmu_pagetable;

/* Access control specifiers
 *  * NA = No Access, RO = Read Only, RW = Read/Write */
#define MMU_RCB_NANA    0x00
#define MMU_RCB_RWNA    0x01
#define MMU_RCB_RWRO    0x02
#define MMU_RCB_RWRW    0x03

/**
 * Region Control Block - RCB */
typedef struct {
    uint32_t vAddr;     /*!< is the starting address of thr region in virtual
                          memory. */
    uint32_t pageSize;  /*!< size of a virtual page. */
    uint32_t numPages;  /*!< is the number of pages in the region. */
    uint32_t ap;        /*!< selects the region access permissions. */
    uint32_t cb;        /*!< selects the cache and write buffer attributes. */
    uint32_t pAddr;     /*!< is the starting address of the region in virtual
                          memory. */
    mmu_pagetable * pt; /*!< is a pointer to the page table in which the region
                          resides. */
} mmu_region;



#endif /* ARM11_MMU_H */

/**
  * @}
  */

/**
  * @}
  */
