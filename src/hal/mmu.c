/**
 *******************************************************************************
 * @file    mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control.
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

/** @addtogroup HAL
  * @{
  */

#include <kinit.h>
#include <kstring.h>
#include <kerror.h>
//#include <ptmapper.h>
#include "mmu.h"

void mmu_init(void);
HW_PREINIT_ENTRY(mmu_init);

/**
 * Initialize the MMU and static regions.
 * @note This is called from startup.
 */
void mmu_init(void)
{
    uint32_t value, mask;

    SUBSYS_INIT();
    SUBSYS_DEP(interrupt_preinit);
    SUBSYS_DEP(ptmapper_init);

    KERROR(KERROR_LOG, "MMU init");

    /* Set MMU_DOM_KERNEL as client and others to generate error. */
    value = MMU_DOMAC_TO(MMU_DOM_KERNEL, MMU_DOMAC_CL);
    mask = MMU_DOMAC_ALL;
    mmu_domain_access_set(value, mask);

    KERROR(KERROR_DEBUG, "Enabling MMU");
    value = MMU_ZEKE_C1_DEFAULTS;
    mask = MMU_ZEKE_C1_DEFAULTS;
    mmu_control_set(value, mask);

    KERROR(KERROR_LOG, "MMU init OK");
}

/**
 * Get size of a page table.
 * @param pt is the page table.
 * @return Return the size of the page table or zero in case of error.
 */
size_t mmu_sizeof_pt(mmu_pagetable_t * pt)
{
    size_t size;

    switch (pt->type) {
        case MMU_PTT_MASTER:
            size = MMU_PTSZ_MASTER;
            break;
        case MMU_PTT_COARSE:
            size = MMU_PTSZ_COARSE;
            break;
        default:
            size = 0;
    }

    return size;
}

/**
 * Calculate size of a vm region.
 * @param region is a pointer to the region control block.
 * @return Returns the size of the region in bytes.
 */
size_t mmu_sizeof_region(mmu_region_t * region)
{
    if (!region->pt)
        return 0;

    switch(region->pt->type) {
        case MMU_PTT_COARSE:
            return region->num_pages * 4096; /* Cool guys hard code values. */
        case MMU_PTT_MASTER:
            return region->num_pages * 1024 * 1024;
        default:
            return 0;
    }
}

/**
 * Clone contents of src page table to dest page table.
 * @param dest  is the destination page table descriptor.
 * @param src   is the source page table descriptor.
 * @return      Zero if cloning succeeded; Otherwise value other than zero.
 */
int mmu_ptcpy(mmu_pagetable_t * dest, mmu_pagetable_t * src)
{
    size_t len_src;
    size_t len_dest;

    len_src = mmu_sizeof_pt(src);
    if (len_src == 0) {
        KERROR(KERROR_ERR, "Attemp to clone an source invalid page table.");
        return 1;
    }

    len_dest = mmu_sizeof_pt(dest);
    if (len_dest == 0) {
        KERROR(KERROR_ERR, "Invalid destination page table.");
        return 1;
    }

    if (len_src != len_dest) {
        KERROR(KERROR_ERR, "Destination and source pts differ in size");
        return 2;
    }

    memcpy((void *)(dest->pt_addr), (void *)(src->pt_addr), len_src);

    return 0;
}

/**
  * @}
  */
