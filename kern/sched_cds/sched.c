/**
 *******************************************************************************
 * @file    sched.c
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#define KERNEL_INTERNAL 1
#include <autoconf.h>
#include <stdint.h>
#include <stddef.h>
#include <kstring.h>
#include <libkern.h>
#include <kmalloc.h>
#include <kinit.h>
#include <sys/linker_set.h>
#include <sys/tree.h>
#include <dllist.h>
#include <sys/sysctl.h>
#include <syscall.h>
#include <kerror.h>
#include <errno.h>
#include <lavg.h>
#include <pthread.h>
#include <thread.h>
#include <tsched.h>

/*
 * Scheduler Queues
 * ================
 *
 * Type Name    Indexing    Description
 * ------------------------------------
 * rb   threads tid         Add threads in the system.
 * rb   ready   nice        Ready for next epoch.
 * rb   exec    time used   Execution queue of the current epoch.
 */
struct cpusched {
    unsigned nr_threads;    /*!< Number of threads in scheduling. */
    unsigned nr_exec;       /*!< Number of threads in execution. */
    int cnt_epoch;
    struct sched_threads    all_threads;
    struct sched_ready      q_ready;
    struct sched_exec       q_exec;
    struct llist          * q_fifo_exec;
};
static struct cpusched cpusched;

/* sysctl node for scheduler */
SYSCTL_DECL(_kern_sched);
SYSCTL_NODE(_kern, OID_AUTO, sched, CTLFLAG_RW, 0, "Scheduler");

SYSCTL_UINT(_kern_sched, OID_AUTO, nr_threads, CTLFLAG_RD,
            &cpusched.nr_threads, 0, "Number of threads.");

static unsigned epoch_len = configSCHED_CDS_EPOCHLEN;
SYSCTL_UINT(_kern_sched, OID_AUTO, epoch_len, CTLFLAG_RW,
            &epoch_len, 0, "Length of scheduler epoch in ticks.");

/** Pointer to the currently active thread */
struct thread_info * current_thread;

/* CPU load averages */
static rwlock_t loadavg_lock;
static uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/** Stack for idle thread */
static char sched_idle_stack[sizeof(sw_stack_frame_t)
                             + sizeof(hw_stack_frame_t)
                             + 40];


static int calc_quantums(struct thread_info * thread);
static void insert_threads(int quantums);
static int fifo_sched(void);
static int cds_sched(void);

void sched_init(void) __attribute__((constructor));
void sched_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(vralloc_init);

    RB_INIT(&cpusched.all_threads);
    RB_INIT(&cpusched.q_ready);
    RB_INIT(&cpusched.q_exec);
    cpusched.q_fifo_exec = dllist_create(struct thread_info,
                                         sched.fifo_exec_entry);

    pthread_t tid;
    pthread_attr_t attr = {
        .tpriority  = NICE_IDLE,
        .stackAddr  = sched_idle_stack,
        .stackSize  = sizeof(sched_idle_stack)
    };
    /* Create the idle task as task 0 */
    struct _ds_pthread_create tdef_idle = {
        .thread   = &tid,
        .start    = idle_thread,
        .def      = &attr,
        .argument = NULL
    };
    /* TODO Exec idle thread */

    /* Initialize locks */
    rwlock_init(&loadavg_lock);

    SUBSYS_INITFINI("Init scheduler: cds");
}

RB_GENERATE(sched_threads, thread_info, sched.threads_entry, sched_tid_comp);
RB_GENERATE(sched_ready, thread_info, sched.ready_entry, sched_nice_comp);
RB_GENERATE(sched_exec, thread_info, sched.exec_entry, sched_ts_comp);

/**
 * Schedule in the next thread.
 * A new epoch starts after the limited number of time quantums is consumed. At
 * the begining of a new epoch the amount of time quantums that will be
 * distributed is decided and threads are taken into execution based on amount
 * of time quantums available.
 *
 * Turn is always given to the thread that has used least time quantums and
 * still has some unused.
 */
void sched_schedule(void)
{
    struct thread_info * thread;
    int (*schedpol[])(void) = {
        fifo_sched, /* RT sched. */
        cds_sched   /* conv sched. */
    };

    current_thread->ts_counter++;

    /* If new epoch begins. */
    if (--cpusched.cnt_epoch <= 0) {
        cpusched.cnt_epoch = epoch_len;
        cpusched.nr_exec = 0;

        insert_threads(epoch_len);
    }

    /* Scheduling */
    current_thread = NULL;
    do {
        for (int i = 0; i < num_elem(schedpol); i++) {
            if (schedpol[i]())
                break;
        }
    } while (!current_thread && (insert_threads(cpusched.cnt_epoch), 1));
}

static int calc_quantums(struct thread_info * thread)
{
    const int fmul = 50; /* Fixed point: 1 / (NICE_MAX + -NICE_MIN + 1) */
    const int add = NICE_MAX + 1;

    return (epoch_len * fmul * (add + thread->niceval)) >> FSHIFT;
}

/**
 * Select ready threads for execution.
 */
static void insert_threads(int quantums)
{
    struct thread_info * thread, * nxt;

    if (!RB_EMPTY(&cpusched.q_ready)) {
        for (thread = RB_MIN(sched_ready, &cpusched.q_ready);
                thread != NULL && quantums > 0;
                thread = nxt) {
            if (thread->priority > quantums)
                return;

            RB_REMOVE(sched_ready, &cpusched.q_ready, thread);

            switch (thread->sched.policy) {
            case SCHED_FIFO:
                cpusched.q_fifo_exec->insert_tail(cpusched.q_fifo_exec, thread);
            case SCHED_OTHER:
                RB_INSERT(sched_exec, &cpusched.q_exec, thread);
                thread->priority = calc_quantums(thread);
                quantums -= thread->priority;
            default:
                panic("Incorrect sched policy.");
            }

            cpusched.nr_exec++;
        }
    }

    if (RB_EMPTY(&cpusched.q_ready)) {
        /* No threads to execute. */
        RB_INSERT(sched_exec, &cpusched.q_exec, sched_get_thread_info(0));
    }
}

/**
 *  RT Scheduling.
 */
static int fifo_sched(void)
{
    struct thread_info * thread;

    while (cpusched.q_fifo_exec->count) {
        thread = cpusched.q_fifo_exec->head;

        if (SCHED_TEST_CSW_OK(thread->flags)) {
            current_thread = thread;

            return 1;
        } else {
            cpusched.q_fifo_exec->remove(cpusched.q_fifo_exec, thread);

            if (SCHED_TEST_DETACHED_ZOMBIE(thread->flags)) {
                thread_terminate(thread->id);
            }
        }
    }

    return 0;
}

/**
 * Conventional Scheduling.
 */
static int cds_sched(void)
{
    struct thread_info * thread;

    do {
        thread = RB_MIN(sched_exec, &cpusched.q_exec);

        if (thread->ts_counter < thread->priority &&
                SCHED_TEST_CSW_OK(thread->flags)) {
            current_thread = thread;

            return 1;
        } else {
            RB_REMOVE(sched_exec, &cpusched.q_exec, thread);

            if (SCHED_TEST_DETACHED_ZOMBIE(thread->flags)) {
                thread_terminate(thread->id);
            }
        }
    } while (!current_thread);

    return 0;
}

static void sched_calc_loads(void)
{
    static int count = LOAD_FREQ;
    uint32_t nr_active; /* Fixed-point */

    /* Run only on kernel tick */
    if (!flag_kernel_tick)
        return;

    count--;
    if (count < 0) {
        if (rwlock_trywrlock(&loadavg_lock) == 0) {
            count = LOAD_FREQ; /* Count is only reset if we get write lock so
                                * we can try again each time. */
            nr_active  = (uint32_t)(cpusched.nr_exec * FIXED_1);

            /* Load averages */
            CALC_LOAD(loadavg[0], FEXP_1, nr_active);
            CALC_LOAD(loadavg[1], FEXP_5, nr_active);
            CALC_LOAD(loadavg[2], FEXP_15, nr_active);

            rwlock_wrunlock(&loadavg_lock);

            /* On the following lines we cheat a little bit to get the write
             * lock faster. This should be ok as we know that this function
             * is the only one trying to write. */
            loadavg_lock.wr_waiting = 0;
        } else if (loadavg_lock.wr_waiting == 0) {
            loadavg_lock.wr_waiting = 1;
        }
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

pthread_t sched_new_tid(void)
{
    /* TODO Create if doesn't exist */
    return -1;
}

struct thread_info * sched_get_thread_info(pthread_t thread_id)
{
    struct thread_info find;
    struct thread_info * thread;

    find.id = thread_id;
    thread = RB_FIND(sched_threads, &cpusched.all_threads, &find);

    return thread;
}

/* TODO */
pthread_t sched_threadCreate(struct _ds_pthread_create * thread_def, int priv)
{
    return -1;
}

int sched_thread_set_priority(pthread_t thread_id, int priority)
{
    struct thread_info * tp = sched_get_thread_info(thread_id);

    if (!tp || (tp->flags & SCHED_IN_USE_FLAG) == 0) {
        return -ESRCH;
    }

    /* Only thread niceval is updated to make this syscall O(1)
     * Actual priority will be updated anyway some time later after one sleep
     * cycle.
     */
    tp->niceval = priority;

    return 0;
}

int sched_thread_get_priority(pthread_t thread_id)
{
    struct thread_info * tp = sched_get_thread_info(thread_id);

    if (!tp || (tp->flags & SCHED_IN_USE_FLAG) == 0) {
        return NICE_ERR;
    }

    /*
     * TODO Not sure if this function should return "dynamic" priority or
     * default priorty.
     */
    return tp->niceval;
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
