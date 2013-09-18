/**
 *******************************************************************************
 * @file    arm11_mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control functions for ARM11 ARMv6 instruction set.
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

#include "../mmu.h"

/** Base address of Page table region */
#define MMU_PT_BASE MMU_TTBR_ADDR

/** Size of static L1 tables */
#define MMU_PT_L1TABLES (MMU_PTSZ_MASTER + MMU_PTSZ_PROC)

/**
 * A macro to calculate the address for statically allocated page table.
 *
 * Note: We assume that there is only one master table and all other tables
 * are equally sized.
 */
#define MMU_PT_ADDR(index) (MMU_PT_BASE + MMU_PT_L1TABLES + index * MMU_PTSZ_COARSE)

/**
 * Last static page table index.
 */
#define MMU_PT_LAST_SINDEX 1

/**
 * First dynamic page table address.
 */
#define MMU_PT_FIRST_DYNPT MMU_PT_ADDR(MMU_PT_LAST_SINDEX + 1)

/* TODO move to memmap */
#define MMU_PT_SHARED_START 0x4000000

/* Fixed page tables */

/* Master, allocated directly on L1 */
mmu_pagetable_t mmu_pagetable_master = {
    .vAddr          = MMU_PT_BASE,
    .ptAddr         = MMU_PT_BASE,
    .masterPtAddr   = MMU_PT_BASE,
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_KERNEL
};

/* L1 TTBR0 Kernel page table */
mmu_pagetable_t mmu_pagetable_kernel_master = {
    .vAddr          = MMU_PT_BASE,
    .ptAddr         = MMU_PT_BASE + MMU_PTSZ_MASTER,
    .masterPtAddr   = MMU_PT_BASE,
    .type           = MMU_PTT_MASTER,
    .dom            = MMU_DOM_KERNEL
};

/* Kernel page table */
mmu_pagetable_t mmu_pagetable_kernel = {
    .vAddr          = 0x0,
    .ptAddr         = MMU_PT_ADDR(0),
    .masterPtAddr   = MMU_PT_BASE + MMU_PTSZ_MASTER,
    .type           = MMU_PTT_COARSE,
    .dom            = MMU_DOM_KERNEL
};

mmu_pagetable_t mmu_pagetable_system = {
    .vAddr          = 0x0,
    .ptAddr         = MMU_PT_ADDR(1),
    .masterPtAddr   = MMU_PT_BASE,
    .type           = MMU_PTT_COARSE,
    .dom            = MMU_DOM_KERNEL
}


/* Regions */

mmu_region_t mmu_region_kernel = {
    .vAddr          = 0x0,
    .numPages       = 32, /* TODO Temporarily mapped as a one area */
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WB | MMU_CTRL_NG,
    .pAddr          = 0x0,
    .pt             = &mmu_pagetable_kernel
}

mmu_region_t mmu_region_page_tables = {
    .vAddr          = MMU_PT_BASE,
    .numPages       = 8, /* TODO 32 megs of page tables?? */
    .ap             = MMU_AP_RWNA,
    .control        = MMU_CTRL_MEMTYPE_WT,
    .pAddr          = MMU_PT_BASE,
    .pt             = &mmu_pagetable_master
}

mmu_region_t mmu_region_shared = {
    .vAddr          = MMU_PT_SHARED_START,
    .numPages       = 4,
    .ap             = MMU_AP_RWRO,
    .control        = MMU_CTRL_MEMTYPE_WT,
    .pAddr          = MMU_PT_SHARED_START,
    .pt             = &mmu_pagetable_master
}


/**
  * @}
  */

/**
  * @}
  */
