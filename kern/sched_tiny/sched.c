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

#define KERNEL_INTERNAL
#include <autoconf.h>
#include <stdint.h>
#include <stddef.h>
#include <kstring.h>
#include <libkern.h>
#include <kinit.h>
#include <sys/linker_set.h>
#include <hal/core.h>
#include <klocks.h>
#include "heap.h"
#include <queue_r.h>
#include <timers.h>
#include <syscall.h>
#include <ptmapper.h>
#include <vm/vm.h>
#include <proc.h>
#include <pthread.h>
#include <sys/sysctl.h>
#include <errno.h>
#include <kerror.h>
#include <lavg.h>
#include <thread.h>
#include <tsched.h>

/* sysctl node for scheduler */
SYSCTL_DECL(_kern_sched);
SYSCTL_NODE(_kern, OID_AUTO, sched, CTLFLAG_RW, 0, "Scheduler");

/* Task containers */
static threadInfo_t task_table[configSCHED_MAX_THREADS]; /*!< Array of all
                                                          *   threads */
static heap_t priority_queue = HEAP_NEW_EMPTY; /*!< Priority queue of active
                                                * threads */
/* Next thread_id queue */
static queue_cb_t next_thread_id_queue_cb;
static pthread_t next_thread_id_queue[configSCHED_MAX_THREADS - 1];
static unsigned max_threads = configSCHED_MAX_THREADS;
SYSCTL_UINT(_kern_sched, OID_AUTO, max_threads, CTLFLAG_RD,
    &max_threads, 0, "Max no. of threads.");
static unsigned nr_threads;
SYSCTL_UINT(_kern_sched, OID_AUTO, nr_threads, CTLFLAG_RD,
    &nr_threads, 0, "Number of threads.");

threadInfo_t * current_thread; /*!< Pointer to the currently active
                                *   thread */
static rwlock_t loadavg_lock;
static uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/** Stack for idle thread */
static char sched_idle_stack[sizeof(sw_stack_frame_t)
                             + sizeof(hw_stack_frame_t)
                             + 40];

/* Static function declarations **********************************************/
static void init_thread_id_queue(void);
static void _sched_thread_set_exec(pthread_t thread_id, int pri);
/* End of Static function declarations ***************************************/

/* Functions called from outside of kernel context ***************************/

/**
 * Initialize the scheduler
 */
void sched_init(void) __attribute__((constructor));
void sched_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(vralloc_init);

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

    thread_init(&task_table[0], 0, &tdef_idle, NULL, 1);
    current_thread = 0; /* To initialize it later on sched_handler. */

    /* Initialize locks */
    rwlock_init(&loadavg_lock);

    /* Initialize threadID queue */
    init_thread_id_queue();

    SUBSYS_INITFINI("Init scheduler: tiny");
}

/**
 * Initialize threadId queue.
 */
static void init_thread_id_queue(void)
{
    pthread_t i = 1;
    int q_ok;

    next_thread_id_queue_cb = queue_create(next_thread_id_queue, sizeof(pthread_t),
            sizeof(next_thread_id_queue));

    /* Fill the queue */
    do {
        q_ok = queue_push(&next_thread_id_queue_cb, &i);
        i++;
    } while (q_ok);
}

/* End of Functions called from outside of kernel context ********************/

/**
 * Idle task specific for this scheduler.
 */
static void idle_task(void)
{
    unsigned tmp_nr_threads = 0;

    /* Update nr_threads */
    for (int i = 0; i < configSCHED_MAX_THREADS; i++) {
        if (task_table[i].flags & SCHED_IN_USE_FLAG) {
            tmp_nr_threads++;
        }
        nr_threads = tmp_nr_threads;
    }

    idle_sleep();
}
DATA_SET(sched_idle_tasks, idle_task);

/**
 * Calculate load averages
 *
 * This function calculates unix-style load averages for the system.
 * Algorithm used here is similar to algorithm used in Linux.
 */
static void sched_calc_loads(void)
{
    static int count = LOAD_FREQ;
    uint32_t active_threads; /* Fixed-point */

    /* Run only on kernel tick */
    if (!flag_kernel_tick)
        return;

    count--;
    if (count < 0) {
        if (rwlock_trywrlock(&loadavg_lock) == 0) {
            count = LOAD_FREQ; /* Count is only reset if we get write lock so
                                * we can try again each time. */
            active_threads = (uint32_t)(priority_queue.size * FIXED_1);

            /* Load averages */
            CALC_LOAD(loadavg[0], FEXP_1, active_threads);
            CALC_LOAD(loadavg[1], FEXP_5, active_threads);
            CALC_LOAD(loadavg[2], FEXP_15, active_threads);

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

/**
 * Schedule next thread.
 */
void sched_schedule(void)
{
    do {
        /* Get next thread from the priority queue */
        current_thread = *priority_queue.a;

        if (!SCHED_TEST_CSW_OK(current_thread->flags)) {
            /* Remove the top thread from the priority queue as it is
             * either in sleep or deleted */
            (void)heap_del_max(&priority_queue);

            if (SCHED_TEST_DETACHED_ZOMBIE(current_thread->flags)) {
                thread_terminate(current_thread->id);
                current_thread = 0;
            }
            continue; /* Select next thread */
        } else if ( /* if maximum time slices for this thread is used */
                    (current_thread->ts_counter <= 0) &&
                    /* and process is not a realtime process */
                    (current_thread->priority < NICE_MAX)
                    /* and its priority is yet better than minimum */
                    && (current_thread->priority > NICE_MIN))
        {
            /* Give a penalty: Set lower priority
             * and perform reschedule operation on heap. */
            heap_reschedule_root(&priority_queue, NICE_MIN);

            continue; /* Select next thread */
        }

        /* Thread is skipped if its EXEC or IN_USE flags are unset. */
    } while (!SCHED_TEST_CSW_OK(current_thread->flags));

    /* ts_counter is used to determine how many time slices has been used by the
     * process between idle/sleep states. We can assume that this measure is
     * quite accurate even it's not confirmed that one tick has been elapsed
     * before this line. */
    current_thread->ts_counter--;
}

pthread_t sched_new_tid(void)
{
    pthread_t new_id;

    if (!queue_pop(&next_thread_id_queue_cb, &new_id)) {
        return -EAGAIN;
    }

    return new_id;
}

struct thread_info * sched_get_thread_info(pthread_t thread_id)
{
    if (thread_id > configSCHED_MAX_THREADS)
        return NULL;

    if ((task_table[thread_id].flags & (uint32_t)SCHED_IN_USE_FLAG) == 0)
        return NULL;

    return &(task_table[thread_id]);
}

void sched_thread_set_exec(pthread_t thread_id)
{
    _sched_thread_set_exec(thread_id, task_table[thread_id].niceval);
}

/**
  * Set thread into execution mode/ready to run mode
  *
  * Sets EXEC_FLAG and puts thread into the scheduler's priority queue.
  * @param thread_id    Thread id
  * @param pri          Priority
  */
static void _sched_thread_set_exec(pthread_t thread_id, int pri)
{
    istate_t s;

    /* Check that given thread is in use but not in execution */
    if (SCHED_TEST_WAKEUP_OK(task_table[thread_id].flags)) {
        s = get_interrupt_state();
        disable_interrupt(); /* TODO Not MP safe! */

        task_table[thread_id].ts_counter = (-NICE_PENALTY + pri) >> 1;
        task_table[thread_id].priority = pri;
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
        (void)heap_insert(&priority_queue, &(task_table[thread_id]));

        set_interrupt_state(s);
    }
}

void sched_sleep_current_thread(int permanent)
{
    disable_interrupt(); /* TODO Not MP safe! */

    current_thread->flags &= ~SCHED_EXEC_FLAG;
    current_thread->flags |= SCHED_WAIT_FLAG;
    if (permanent) {
        atomic_set(&current_thread->a_wait_count, -1);
    }

    current_thread->priority = NICE_ERR;
    heap_inc_key(&priority_queue, heap_find(&priority_queue,
                current_thread->id));

    /* We don't want to get stuck here */
    enable_interrupt();

    while (current_thread->flags & SCHED_WAIT_FLAG)
        idle_sleep();
}

void sched_current_thread_yield(int sleep_flag)
{
    if (!current_thread || !(*priority_queue.a))
        return;

    if ((*priority_queue.a)->id == current_thread->id)
        heap_reschedule_root(&priority_queue, NICE_YIELD);

    if (sleep_flag)
        idle_sleep();

    /* TODO User may expect this function to yield immediately which doesn't
     * actually happen. */
}

void sched_thread_remove(pthread_t tt_id)
{
    if ((task_table[tt_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return; /* Already freed */
    }

    task_table[tt_id].flags = 0; /* Clear all flags */

    /* Increment the thread priority to the highest possible value so context
     * switch will garbage collect it from the priority queue on the next run.
     */
    task_table[tt_id].priority = NICE_ERR;
    {
        int i;
        i = heap_find(&priority_queue, tt_id);
        if (i != -1)
            heap_inc_key(&priority_queue, i);
    }

    /* Release thread id */
    queue_push(&next_thread_id_queue_cb, &tt_id);

#if 0
    set_interrupt_state(s);
#endif
}

int sched_thread_detach(pthread_t thread_id)
{
    struct thread_info * tp = &task_table[thread_id];

    if ((tp->flags & SCHED_IN_USE_FLAG) == 0) {
        return -EINVAL;
    }

    tp->flags |= SCHED_DETACH_FLAG;
    if (SCHED_TEST_DETACHED_ZOMBIE(tp->flags)) {
        /* Workaround to remove the thread without locking the system now
         * because we don't want to disable interrupts here for too long time
         * and we have no other proctection in the scheduler right now. */
        istate_t s;
        int i;

        /* TODO This part is not quite clever nor MP safe. */
        s = get_interrupt_state();
        disable_interrupt();

        i = heap_find(&priority_queue, thread_id);

        if (i < 0)
            (void)heap_insert(&priority_queue, tp);
        /* It will be now definitely gc'ed at some point. */
        set_interrupt_state(s);
    }

    return 0;
}

/* Functions defined in header file
 ******************************************************************************/

/*  ==== Thread Management ==== */

pthread_t sched_thread_create(struct _ds_pthread_create * thread_def, int priv)
{
    pthread_t i;
#if 0
    istate_t s;

    s = get_interrupt_state();
    disable_interrupt();
#endif

    i = sched_new_tid();
    if (i < 0) {
        /* New thread could not be created */
#if 0
        set_interrupt_state(s);
#endif
        return -1;
    }

    thread_init(
            &task_table[i],
            i,                      /* Index of the thread created */
            thread_def,             /* Thread definition. */
            (void *)current_thread, /* Pointer to the parent thread, which is
                                     * expected to be the current thread. */
            priv);                  /* kworker flag. */

#if 0
        set_interrupt_state(s);
#endif

    if (i == configSCHED_MAX_THREADS) {
        /* New thread could not be created */
        return -2;
    } else {
        /* Return the id of the new thread */
        return i;
    }
}

int sched_thread_set_priority(pthread_t thread_id, int priority)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return -ESRCH;
    }

    /* Only thread niceval is updated to make this syscall O(1)
     * Actual priority will be updated anyway some time later after one sleep
     * cycle.
     */
    task_table[thread_id].niceval = priority;

    return 0;
}

int sched_thread_get_priority(pthread_t thread_id)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return NICE_ERR;
    }

    /*
     * TODO Not sure if this function should return "dynamic" priority or
     * default priorty.
     */
    return task_table[thread_id].niceval;
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
