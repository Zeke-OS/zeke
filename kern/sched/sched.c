/**
 *******************************************************************************
 * @file    sched.c
 * @author  Olli Vanhoja
 * @brief   Scheduler dispatcher.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <sys/linker_set.h>
#include <sys/tree.h>
#include <sys/sysctl.h>
#include <syscall.h>
#include <errno.h>
#include <libkern.h>
#include <kstring.h>
#include <kinit.h>
#include <kerror.h>
#include <buf.h>
#include <kmalloc.h>
#include <timers.h>
#include <proc.h>
#include <idle.h>
#include <ksched.h>

/*
 * Load average calculation.
 * FEXP_N = 2^11/(2^(interval * log_2(e/N)))
 */
#if defined(configSCHED_LAVGPERIOD_5SEC)
#define LOAD_FREQ (5 * (int)configSCHED_HZ)
#define FSHIFT      11      /*!< nr of bits of precision */
#define FEXP_1      1884    /*!< 1/exp(5sec/1min) */
#define FEXP_5      2014    /*!< 1/exp(5sec/5min) */
#define FEXP_15     2037    /*!< 1/exp(5sec/15min) */
#elif defined(configSCHED_LAVGPERIOD_11SEC)
#define LOAD_FREQ (11 * (int)configSCHED_HZ)
#define FSHIFT      11
#define FEXP_1      1704
#define FEXP_5      1974
#define FEXP_15     2023
#else
#error Incorrect value of kernel configuration for LAVG
#endif
#define FIXED_1     (1 << FSHIFT) /*!< 1.0 in fixed-point */
#define CALC_LOAD(load, exp, n)                  \
                    load *= exp;                 \
                    load += n * (FIXED_1 - exp); \
                    load >>= FSHIFT;
/** Scales fixed-point load average value to a integer format scaled to 100 */
#define SCALE_LOAD(x) (((x + (FIXED_1 / 200)) * 100) >> FSHIFT)

/* sysctl node for scheduler */
SYSCTL_NODE(_kern, OID_AUTO, sched, CTLFLAG_RW, 0, "Scheduler");

/**
 * Pointer to the currently active thread.
 */
struct thread_info * current_thread;

static rwlock_t loadavg_lock;
static uint32_t loadavg[3] = { 0, 0, 0 }; /*!< CPU load averages */

/* Linker sets for pre- and post-scheduling tasks */
SET_DECLARE(pre_sched_tasks, void);
SET_DECLARE(post_sched_tasks, void);

#if configIDLE_TH_STACK_SIZE < 40
#error Idle thread stack (configIDLE_TH_STACK_SIZE) should be at least 40
#endif
/** Stack for the idle thread */
static char sched_idle_stack[sizeof(sw_stack_frame_t) +
                             sizeof(hw_stack_frame_t) +
                             configIDLE_TH_STACK_SIZE];

/**
 * Array of schedulers in order of execution.
 */
static struct scheduler * sched_arr[] = {
    &sched_rr,
    &sched_idle,
};

extern void _thread_sys_init(void);
extern void * idle_thread(void * arg);

int sched_init(void) __attribute__((constructor));
int sched_init(void)
{
    SUBSYS_DEP(vralloc_init);
    SUBSYS_INIT("sched");

    struct _sched_pthread_create_args tdef_idle = {
        .param.sched_policy   = SCHED_OTHER + 1,
        .param.sched_priority = NZERO,
        .stack_addr = sched_idle_stack,
        .stack_size = sizeof(sched_idle_stack),
        .flags      = 0,
        .start      = idle_thread,
        .arg1       = 0,
        .del_thread = NULL,
    };

    _thread_sys_init();
    thread_create(&tdef_idle, 1);

    /* Initialize locks */
    rwlock_init(&loadavg_lock);

    /* TODO Call scheduler initializers */

    return 0;
}

int sched_csw_ok(struct thread_info * thread)
{
    if (thread_flags_not_set(thread, SCHED_IN_USE_FLAG) ||
        (thread_state_get(thread) != THREAD_STATE_EXEC) ||
        thread->sched.ts_counter == 0)
        return 0;

    return (1 == 1);
}

/**
 * Calculate load averages
 *
 * This function calculates unix-style load averages for the system.
 * Algorithm used here is similar to algorithm used in Linux.
 */
static void sched_calc_loads(void)
{
    static int count = LOAD_FREQ;
    uint32_t active_threads = 0; /* Fixed-point */

    /* Run only on a kernel tick */
    if (!flag_kernel_tick)
        return;

    count--;
    if (count >= 0)
        return;

    if (rwlock_trywrlock(&loadavg_lock) == 0) {
        count = LOAD_FREQ;

        for (size_t i = 0; i < num_elem(sched_arr); i++) {
            unsigned nr;

            nr = sched_arr[i]->get_nr_active_threads();
            active_threads += (uint32_t)nr * FIXED_1;
        }

        /* Load averages */
        CALC_LOAD(loadavg[0], FEXP_1, active_threads);
        CALC_LOAD(loadavg[1], FEXP_5, active_threads);
        CALC_LOAD(loadavg[2], FEXP_15, active_threads);

        rwlock_wrunlock(&loadavg_lock);

        /*
         * On the following lines we cheat a little bit to get the write lock
         * faster. This should be ok as long as we know that this function is
         * the only one trying to write.
         */
        loadavg_lock.wr_waiting = 0;
    } else if (loadavg_lock.wr_waiting == 0) {
        loadavg_lock.wr_waiting = 1;
    }
}
DATA_SET(post_sched_tasks, sched_calc_loads);

void sched_get_loads(uint32_t loads[3])
{
    rwlock_rdlock(&loadavg_lock);
    loads[0] = SCALE_LOAD(loadavg[0]);
    loads[1] = SCALE_LOAD(loadavg[1]);
    loads[2] = SCALE_LOAD(loadavg[2]);
    rwlock_rdunlock(&loadavg_lock);
}

void sched_handler(void)
{
    struct thread_info * const prev_thread = current_thread;
    void ** task_p;

    if (!current_thread) {
        current_thread = thread_lookup(0);
        if (!current_thread)
            panic("No thread 0\n");
    }

    proc_update_times();

    /*
     * Pre-scheduling tasks.
     */
    SET_FOREACH(task_p, pre_sched_tasks) {
        sched_task_t task = *(sched_task_t *)task_p;
        task();
    }

    if (current_thread->sched.ts_counter != -1) {
        current_thread->sched.ts_counter--;
    }

    /*
     * Exhaust global readyq.
     */
    for (struct thread_info * thread = thread_remove_ready();
         thread;
         thread = thread_remove_ready()) {
        thread_state_set(thread, THREAD_STATE_EXEC);
        if (sched_arr[thread->param.sched_policy]->insert(thread)) {
            KERROR(KERROR_ERR, "Failed to schedule a thread (%d)", thread->id);
        }
    }

    /*
     * Run schedulers until next runnable thread is found.
     */
    current_thread = NULL;
    for (size_t i = 0; i < num_elem(sched_arr); i++) {
        sched_arr[i]->run();
        if (current_thread)
            break;
    }
#if configSCHED_DEBUG
    if (!current_thread) {
        panic("Nothing to schedule");
    }
#endif
    if (current_thread != prev_thread) {
        mmu_map_region(&(current_thread->kstack_region->b_mmu));
    }

    /*
     * Post-scheduling tasks
     */
    SET_FOREACH(task_p, post_sched_tasks) {
        sched_task_t task = *(sched_task_t *)task_p;
        task();
    }
}

/* Syscall handlers ***********************************************************/

static int sys_sched_get_loadavg(void * user_args)
{
    uint32_t arr[3];
    int err;

    sched_get_loads(arr);

    err = copyout(arr, user_args, sizeof(arr));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static const syscall_handler_t sched_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SCHED_GET_LOADAVG, sys_sched_get_loadavg)
};
SYSCALL_HANDLERDEF(sched_syscall, sched_sysfnmap)
