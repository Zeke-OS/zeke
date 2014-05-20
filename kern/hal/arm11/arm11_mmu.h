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

/** @addtogroup MMU
 * @{
 */

/** @addtogroup ARM11
 * @{
 */

#pragma once
#ifndef ARM11_MMU_H
#define ARM11_MMU_H

#include <hal/mmu.h>

/**
 * Size of translation table pointed by TTBR0.
 *
 * N    bound   Table size  Entries
 * --------------------------------
 * 0    4GB     16KB        4096    (ARMv5 behaviour)
 * 1    2GB     8KB         2048
 * 2    1GB     4KB         1024
 * 3    512MB   2KB         512
 * 4    256MB   1KB         256
 * 5    128MB   512B        128
 * 6    64MB    256B        64
 * 7    32MB    128B        32
 */
#define MMU_TTBCR_N         0

/* L1 Page Table Entry Types
 * These corresponds directly to the bits of first-level descriptor on ARMv6 */
#define MMU_PTE_FAULT       0 /*!< Translation fault. */
#define MMU_PTE_COARSE      1 /*!< Coarse page table. */
#define MMU_PTE_SECTION     2 /*!< Section entry. */

/* Page table sizes in bytes */
#define MMU_PTSZ_FAULT      0x0000 /*!< Page table size for translation fault. */
#define MMU_PTSZ_COARSE     0x0400 /*!< Coarse page table size. */
#define MMU_PTSZ_MASTER     0x4000 /*!< L1 master page table size. */

/* Page sizes in bytes */
#define MMU_PGSIZE_COARSE   4096    /*!< Size of a coarse page table page. */
#define MMU_PGSIZE_SECTION  1048576 /*!< Size of a master page table section. */

/* Domain Access Control Macros */
#define MMU_DOMAC_NA        0x0 /*!< Any access generates a domain fault. */
#define MMU_DOMAC_CL        0x1 /*!< Client. Access is checked against the ap bits in TLB. */
#define MMU_DOMAC_MA        0x3 /*!< Manager. No access permission checks performed. */

/**
 * Domain number to domain mask.
 * @param domain as a number.
 * @return domain mask suitable for mmu_domain_access_set function.
 */
#define MMU_DOMAC_DOM2MASK(dom)     (0x3 << dom)

/**
 * Mask for all domains.
 */
#define MMU_DOMAC_ALL               0xffffffff

/**
 * Domain Access Control value for dom.
 * @param dom domain.
 * @param val access bits, MMU_DOMAC_NA, MMU_DOMAC_CL or MMU_DOMAC_MA.
 * @return value that can be passed to mmu_domain_access_set function.
 * */
#define MMU_DOMAC_TO(dom, val)      ((val & 0x3) << dom)

/**
 * Get Domain Access Control value of dom from the return value of
 * mmu_domain_access_get function.
 * @param dom which domain.
 * @param val return value from the mmu_domain_access_get function.
 * @return MMU_DOMAC_NA, MMU_DOMAC_CL or MMU_DOMAC_MA.
 */
#define MMU_DOMAC_FROM(dom, val)    ((val >> dom) & 0x3)

/* End of DAC Macros */

/* MMU C1 Control bits
 * This list contains only those settings that are usable with Zeke.
 */
#define MMU_C1_CR_ENMMU     0x00000001 /*!< Enables the MMU. */
#define MMU_C1_CR_DCACHE    0x00000004 /*!< Enables the L1 data cache. */
#define MMU_C1_CR_ICACHE    0x00001000 /*!< Enables the L1 instruction cache. */
#define MMU_C1_CR_BPRED     0x00000800 /*!< Enables branch prediction. */
#define MMU_C1_CR_XP        0x00800000 /*!< Disable AP subpages and enable ARMv6
                                        * extensions */
#define MMU_C1_CR_U         0x00400000 /*!< Unaligned data access operations. */
#define MMU_C1_CR_VE        0x01000000 /*!< Enables the VIC interface */
#define MMU_C1_CR_TR        0x10000000 /*!< Enables TEX remap. */
#define MMU_C1_CR_FA        0x20000000 /*!< Force AP bits */ /* TODO? */
/** Default MMU C1 configuration for Zeke */
#define MMU_ZEKE_C1_DEFAULTS    (MMU_C1_CR_ENMMU | MMU_C1_CR_DCACHE |\
        MMU_C1_CR_ICACHE | MMU_C1_CR_XP | MMU_C1_CR_VE | MMU_C1_CR_TR)
/* End of MMU C1 Control Bits */

#endif /* ARM11_MMU_H */

/**
 * @}
 */


/**
 * @}
 */

/**
 * @}
 */
