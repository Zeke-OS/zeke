/**
 *******************************************************************************
 * @file    mmu.c
 * @author  Olli Vanhoja
 * @brief   MMU control.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <sys/sysctl.h>
#include <sys/linker_set.h>
#include <hal/mmu.h>
#include <kerror.h>
#include <klocks.h>
#include <kstring.h>
#include <proc.h>

/* Definitions for Page fault counter *****************************************/
#define PFC_FREQ    (int)configSCHED_HZ /* We wan't to compute pf/s once per
                                         * second. */
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

static unsigned long _pf_raw_count; /*!< Raw page fault counter. */
#ifdef configMP
/** Mutex for _pf_raw_count */
static mtx_t _pfrc_lock = MTX_INITIALIZER(MTX_TYPE_SPIN, MTX_OPT_DEFAULT);
#endif
unsigned long mmu_pfps; /*!< Page faults per second average. Fixed point, 11
                         *   bits. */
SYSCTL_UINT(_vm, OID_AUTO, pfps, CTLFLAG_RD, (&mmu_pfps), 0,
    "Page faults per second average.");

size_t mmu_sizeof_pt(const mmu_pagetable_t * pt)
{
    size_t nr_tables;

    /* TODO Transitional */
    nr_tables = (pt->nr_tables == 0) ? 1 : pt->nr_tables;

    switch (pt->pt_type) {
    case MMU_PTT_MASTER:
        return nr_tables * MMU_PTSZ_MASTER;
        break;
    case MMU_PTT_COARSE:
        return nr_tables * MMU_PTSZ_COARSE;
        break;
    default:
        KERROR(KERROR_ERR,
               "mmu_sizeof_pt(%p) failed, pt is uninitialized\n", pt);
#ifdef configMMU_DEBUG
        KERROR_DBG_PRINT_RET_ADDR();
#endif
        return 0;
    }
}

size_t mmu_sizeof_pt_img(const mmu_pagetable_t * pt)
{
    size_t nr_tables;

    /* TODO Transitional */
    nr_tables = (pt->nr_tables == 0) ? 1 : pt->nr_tables;

    switch (pt->pt_type) {
    case MMU_PTT_MASTER:
        return MMU_NR_SECTION_ENTR * MMU_PGSIZE_SECTION;
        break;
    case MMU_PTT_COARSE:
        return nr_tables * MMU_PGSIZE_SECTION;
        break;
    default:
        KERROR(KERROR_ERR,
               "mmu_sizeof_pt(%p) failed, pt is uninitialized\n", pt);
#ifdef configMMU_DEBUG
        KERROR_DBG_PRINT_RET_ADDR();
#endif
        return 0;
    }
}

size_t mmu_sizeof_region(const mmu_region_t * region)
{
    const size_t num_pages = region->num_pages;

    if (!region->pt) {
#ifdef configMMU_DEBUG
        KERROR(KERROR_WARN, "pt for region %p not set\n", region);
        KERROR_DBG_PRINT_RET_ADDR();
#endif
        return 0;
    }

    switch (region->pt->pt_type) {
    case MMU_PTT_COARSE:
        return num_pages * MMU_PGSIZE_COARSE;
    case MMU_PTT_MASTER:
        return num_pages * MMU_PGSIZE_SECTION;
    default:
#ifdef configMMU_DEBUG
        KERROR(KERROR_ERR,
               "mmu_sizeof_region(%p) failed, region is uninitialized\n",
               region);
        KERROR_DBG_PRINT_RET_ADDR();
#endif
        return 0;
    }
}

/**
 * Clone contents of src page table to dest page table.
 * @param dest  is the destination page table descriptor.
 * @param src   is the source page table descriptor.
 * @return      Zero if cloning succeeded; Otherwise value other than zero.
 */
int mmu_ptcpy(mmu_pagetable_t * dest, const mmu_pagetable_t * src)
{
    const size_t len_src = mmu_sizeof_pt(src);
    const size_t len_dest = mmu_sizeof_pt(dest);

    if (len_src == 0) {
        KERROR(KERROR_ERR, "Attemp to clone an source invalid page table.\n");
        return -1;
    }

    if (len_dest == 0) {
        KERROR(KERROR_ERR, "Invalid destination page table.\n");
        return -2;
    }

    if (len_src != len_dest) {
        KERROR(KERROR_ERR, "Destination and source pts differ in size\n");
        return -3;
    }

    memcpy((void *)(dest->pt_addr), (void *)(src->pt_addr), len_src);

    return 0;
}

void mmu_die_on_fatal_abort(void)
{
    enable_interrupt();
    idle_sleep();
}

static const char * get_abo_type_str(const struct mmu_abo_param * restrict abo)
{
    switch (abo->abo_type) {
    case MMU_ABO_DATA:
        return "Data Abort";
    case MMU_ABO_PREFETCH:
        return "Prefetch Abort";
    default:
        return "Unkown abort type";
    }
}

void mmu_abo_dump(const struct mmu_abo_param * restrict abo)
{
    int pid;
    const char * proc_name;

    if (abo->proc) {
        pid = abo->proc->pid;
        proc_name = abo->proc->name;
    } else {
        pid = -1;
        proc_name = "";
    }

    KERROR(KERROR_CRIT,
           "Fatal %s:\n"
           "pc: %x\n"
           "(i)fsr: %x (%s)\n"
           "(i)far: %x\n"
           "proc info:\n"
           "pid: %i\n"
           "tid: %i\n"
           "name: %s\n"
           "insys: %i\n",
           get_abo_type_str(abo),
           abo->lr,
           abo->fsr, mmu_abo_strerror(abo),
           abo->far,
           pid,
           (int32_t)abo->thread->id,
           proc_name,
           (int32_t)thread_flags_is_set(abo->thread, SCHED_INSYS_FLAG));
    /* FIXME Get the sframe from HAL */
    stack_dump(abo->thread->sframe.s[SCHED_SFRAME_ABO]);
}

/**
 * Signal a page fault event for the pf/s counter.
 */
void mmu_pf_event(void)
{
#ifdef configMP
    /*
     * By using spinlock here there should be no risk of deadlock because even
     * that this event is basically called only when one core is in interrupts
     * disabled state the call should not ever nest. If it nests something
     * is badly broken anyway. E.g. it could nest if this function would cause
     * another abort.
     */
    mtx_spinlock(&_pfrc_lock);
#endif

    _pf_raw_count++;

#ifdef configMP
    mtx_unlock(&_pfrc_lock);
#endif
}

/**
 * Calculate pf/s average.
 * This function should be called periodically by the scheduler.
 */
static void mmu_calc_pfcps(void)
{
    static int count = PFC_FREQ;

    /* Run only on kernel tick */
    if (!flag_kernel_tick)
        return;

    /* Tanenbaum suggests in one of his books that pf/s count could be first
     * averaged and then on each iteration summed with the current value and
     * divided by two. We do only the averaging here by the same method used
     * for loadavg.
     */

    count--;
    if (count < 0) {
        unsigned long pfc;

        count = PFC_FREQ;
        pfc = (_pf_raw_count * FIXED_1);
        CALC_PFC(mmu_pfps, pfc);
        _pf_raw_count = 0;
    }
}
DATA_SET(post_sched_tasks, mmu_calc_pfcps);
