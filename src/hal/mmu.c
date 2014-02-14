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

#define KERNEL_INTERNAL
#include <kinit.h>
#include <kstring.h>
#include <kerror.h>
#include <klocks.h>
#include <sys/sysctl.h>
#include "mmu.h"

/* Definitions for Page fault counter *****************************************/
#define PFC_FREQ    (int)configSCHED_HZ
#define FSHIFT      11              /*!< nr of bits of precision */
#define FEXP_1      753             /*!< 1 sec */
#define FIXED_1     (1 << FSHIFT)   /*!< 1.0 in fixed-point */
#define CALC_PFC(avg, n)                           \
                    avg *= FEXP_1;                 \
                    avg += n * (FIXED_1 - FEXP_1); \
                    avg >>= FSHIFT;
/** Scales fixed-point pfc/s average value to a integer format scaled to 100 */
//#define SCALE_PFC(x) (((x + (FIXED_1/200)) * 100) >> FSHIFT)
/* End of Definitions for Page fault counter **********************************/

static unsigned long _pf_raw_count;
#if configMP != 0
static mtx_t _pfrc_lock; /*!< Mutex for _pf_raw_count */
#endif
unsigned long mmu_pfps; /* Page faults per second average. Fixed point, 11 bits. */
SYSCTL_UINT(_vm, OID_AUTO, pfps, CTLFLAG_RD, (&mmu_pfps), 0,
    "Page faults per second average.");

extern void mmu_lock_init();
void mmu_init(void);
HW_PREINIT_ENTRY(mmu_init);

/**
 * Initialize the MMU and static regions.
 * @note This is called from startup.
 */
void mmu_init(void)
{
    SUBSYS_INIT("MMU init");
    SUBSYS_DEP(arm_interrupt_preinit);
    SUBSYS_DEP(ptmapper_init);

    uint32_t value, mask;

#if configMP != 0
    mmu_lock_init();
    mtx_init(&_pfrc_lock, MTX_DEF | MTX_SPIN);
#endif

    /* Set MMU_DOM_KERNEL as client and others to generate error. */
    value = MMU_DOMAC_TO(MMU_DOM_KERNEL, MMU_DOMAC_CL);
    mask = MMU_DOMAC_ALL;
    mmu_domain_access_set(value, mask);

#if configDEBUG != 0
    KERROR(KERROR_DEBUG, "Enabling MMU");
#endif
    value = MMU_ZEKE_C1_DEFAULTS;
    mask = MMU_ZEKE_C1_DEFAULTS;
    mmu_control_set(value, mask);

#if configDEBUG != 0
    KERROR(KERROR_LOG, "MMU init OK");
#endif
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
        return -1;
    }

    len_dest = mmu_sizeof_pt(dest);
    if (len_dest == 0) {
        KERROR(KERROR_ERR, "Invalid destination page table.");
        return -1;
    }

    if (len_src != len_dest) {
        KERROR(KERROR_ERR, "Destination and source pts differ in size");
        return -2;
    }

    memcpy((void *)(dest->pt_addr), (void *)(src->pt_addr), len_src);

    return 0;
}

static unsigned long _pf_raw_count;
static mtx_t _pfrc_mtx;

/**
 * Signal a page fault event for the pf/s counter.
 */
void mmu_pf_event(void)
{
#if configMP != 0
    mtx_spinlock(&_pfrc_lock);
#endif

    _pf_raw_count++;

#if configMP != 0
    mtx_unlock(&_pfrc_lock);
#endif
}

/**
 * Calculate pf/s average.
 * This function should be called periodically by the scheduler.
 */
void mmu_calc_pfcps(void)
{
    static int count = PFC_FREQ;
    unsigned long pfc;

    count--;
    if (count < 0) {
        count = PFC_FREQ;
        pfc = (_pf_raw_count * FIXED_1);
        CALC_PFC(mmu_pfps, pfc);
        _pf_raw_count = 0;
    }
}

/**
  * @}
  */
