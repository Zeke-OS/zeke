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

/** @addtogroup Scheduler
  * @{
  */

#include <stdint.h>
#include <stddef.h>
#include <kstring.h>

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <hal/hal_core.h>
#include <hal/hal_mcu.h>
#include "heap.h"
#if configFAST_FORK != 0
#include <queue.h>
#endif
#include <timers.h>
#include <syscall.h>
#include <kernel.h>
#include <pthread.h>
#include <sched.h>

/* Definitions for load average calculation **********************************/
#define LOAD_FREQ   (configSCHED_LAVG_PER * (int)configSCHED_FREQ)
/* FEXP_N = 2^11/(2^(interval * log_2(e/N))) */
#if configSCHED_LAVG_PER == 5
#define FSHIFT      11      /*!< nr of bits of precision */
#define FEXP_1      1884    /*!< 1/exp(5sec/1min) */
#define FEXP_5      2014    /*!< 1/exp(5sec/5min) */
#define FEXP_15     2037    /*!< 1/exp(5sec/15min) */
#elif configSCHED_LAVG_PER == 11
#define FSHIFT      11
#define FEXP_1      1704
#define FEXP_5      1974
#define FEXP_15     2023
#else
#error Incorrect value of kernel configuration configSCHED_LAVG_PER
#endif
#define FIXED_1     (1 << FSHIFT) /*!< 1.0 in fixed-point */
#define CALC_LOAD(load, exp, n)                  \
                    load *= exp;                 \
                    load += n * (FIXED_1 - exp); \
                    load >>= FSHIFT;
/** Scales fixed-point load average value to a integer format scaled to 100 */
#define SCALE_LOAD(x) (((x + (FIXED_1/200)) * 100) >> FSHIFT)
/* End of Definitions for load average calculation ***************************/


/* Task containers */
static threadInfo_t task_table[configSCHED_MAX_THREADS]; /*!< Array of all
                                                          *   threads */
static heap_t priority_queue = HEAP_NEW_EMPTY;   /*!< Priority queue of active
                                                  * threads */
#if configFAST_FORK != 0
static queue_cb_t next_threadId_queue_cb;
static int next_threadId_queue[configSCHED_MAX_THREADS - 1];
#endif

volatile threadInfo_t * current_thread; /*!< Pointer to the currently active
                                         *   thread */
static uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/** Stack for idle task */
static char sched_idle_stack[sizeof(sw_stack_frame_t)
                             + sizeof(hw_stack_frame_t)
                             + 40]; /* Absolute minimum with current idle task
                                     * implementation */

/* Static init */
void sched_init(void) __attribute__((constructor));

/* Static function declarations **********************************************/
void * idleTask(void * arg);
static void calc_loads(void);
static void context_switcher(void);
static void sched_thread_init(int i, ds_pthread_create_t * thread_def, threadInfo_t * parent);
static void sched_thread_set_inheritance(pthread_t i, threadInfo_t * parent);
static void _sched_thread_set_exec(int thread_id, osPriority pri);
static void sched_thread_remove(pthread_t id);
/* End of Static function declarations ***************************************/

/* Functions called from outside of kernel context ***************************/

/**
 * Initialize the scheduler
 */
void sched_init(void)
{
    pthread_t tid;
    pthread_attr_t attr = {
        .tpriority  = osPriorityIdle,
        .stackAddr  = sched_idle_stack,
        .stackSize  = sizeof(sched_idle_stack)
    };
    /* Create the idle task as task 0 */
    ds_pthread_create_t tdef_idle = {
        .thread   = &tid,
        .start    = idleTask,
        .def      = &attr,
        .argument = NULL
    };
    sched_thread_init(0, &tdef_idle, NULL);

    /* Set idle thread as a currently running thread */
    current_thread = &(task_table[0]);

#if configFAST_FORK != 0
    next_threadId_queue_cb = queue_create(next_threadId_queue, sizeof(int),
        sizeof(next_threadId_queue) / sizeof(int));

    /* Fill the queue */
    int i = 1, q_ok;
    do {
        q_ok = queue_push(&next_threadId_queue_cb, &i);
        i++;
    } while (q_ok);
#endif
}

/* End of Functions called from outside of kernel context ********************/

/**
 * Kernel idle task
 * @note sw stacked registers are invalid when this thread executes for the
 * first time.
 */
void * idleTask(/*@unused@*/ void * arg)
{
#ifndef PU_TEST_BUILD
    while(1) {
        idle_sleep();
    }
#endif
}

#ifndef PU_TEST_BUILD
void * sched_handler(void * tsp)
{
    if (tsp != 0) {
        current_thread->sp = tsp;
    }

    /* Pre-scheduling tasks */
    if (flag_kernel_tick) { /* Run only if tick was set */
        timers_run();
    }

    /* Call the actual context switcher function that schedules the next thread.
     */
    context_switcher();

    /* Post-scheduling tasks */
    if (flag_kernel_tick) {
        calc_loads();
    }

    return current_thread->sp;
}
#endif

/**
 * Calculate load averages
 *
 * This function calculates unix-style load averages for the system.
 * Algorithm used here is similar to algorithm used in Linux, although I'm not
 * exactly sure if it's O(1) in current Linux implementation.
 */
static void calc_loads(void)
{
    uint32_t active_threads; /* Fixed-point */
    static int count = LOAD_FREQ;

    count--;
    if (count < 0) {
        count = LOAD_FREQ;
        active_threads = (uint32_t)(priority_queue.size * FIXED_1);

        /* Load averages */
        CALC_LOAD(loadavg[0], FEXP_1, active_threads);
        CALC_LOAD(loadavg[1], FEXP_5, active_threads);
        CALC_LOAD(loadavg[2], FEXP_15, active_threads);
    }
}

void sched_get_loads(uint32_t loads[3])
{
    loads[0] = SCALE_LOAD(loadavg[0]);
    loads[1] = SCALE_LOAD(loadavg[1]);
    loads[2] = SCALE_LOAD(loadavg[2]);
}

/**
 * Selects the next thread.
 */
static void context_switcher(void)
{
    /* Select the next thread */
    do {
        /* Get next thread from the priority queue */
        current_thread = *priority_queue.a;

        if (!SCHED_TEST_CSW_OK(current_thread->flags)) {
            /* Remove the top thread from the priority queue as it is
             * either in sleep or deleted */
            (void)heap_del_max(&priority_queue);
            continue; /* Select next thread */
        } else if ( /* if maximum time slices for this thread is used */
            (current_thread->ts_counter <= 0)
            /* and process is not a realtime process */
            && ((int)current_thread->priority < (int)osPriorityRealtime)
            /* and its priority is yet higher than low */
            && ((int)current_thread->priority > (int)osPriorityLow))
        {
            /* Penalties
             * =========
             * Penalties are given to CPU hog threads (CPU bound) to prevent
             * starvation of other threads. This is done by dynamically lowering
             * the priority (gives less CPU time in CMSIS RTOS) of a thread.
             */

            /* Give a penalty: Set lower priority
             * and perform reschedule operation on heap. */
            heap_reschedule_root(&priority_queue, osPriorityLow);

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

threadInfo_t * sched_get_pThreadInfo(int thread_id)
{
    if (thread_id > configSCHED_MAX_THREADS)
        return NULL;

    if ((task_table[thread_id].flags & (uint32_t)SCHED_IN_USE_FLAG) == 0)
        return NULL;

    return &(task_table[thread_id]);
}

/**
  * Set thread initial configuration
  * @note This function should not be called for already initialized threads.
  * @param i            Thread id
  * @param thread_def   Thread definitions
  * @param parent       Parent thread id, NULL = doesn't have a parent
  * @todo what if parent is stopped before this function is called?
  */
static void sched_thread_init(int i, ds_pthread_create_t * thread_def, threadInfo_t * parent)
{
    /* This function should not be called for already initialized threads. */
    if ((task_table[i].flags & (uint32_t)SCHED_IN_USE_FLAG) != 0)
        return;

    memset(&(task_table[i]), 0, sizeof(threadInfo_t));

    /* Return thread id */
    *(thread_def->thread) = (pthread_t)i;

    /* Init core specific stack frame */
    init_stack_frame(thread_def, pthread_exit);

    /* Mark this thread index as used.
     * EXEC flag is set later in sched_thread_set_exec */
    task_table[i].flags         = (uint32_t)SCHED_IN_USE_FLAG;
    task_table[i].id            = i;
    task_table[i].def_priority  = thread_def->def->tpriority;
    /* task_table[i].priority is set later in sched_thread_set_exec */

    /* Clear signal flags & wait states */
    task_table[i].wait_tim = -1;

    /* Clear events */
    memset(&(task_table[i].event), 0, sizeof(osEvent));

    /* Update parent and child pointers */
    sched_thread_set_inheritance(i, parent);

    /* Update stack pointer */
    task_table[i].sp = (void *)((uint32_t)(thread_def->def->stackAddr)
                                         + thread_def->def->stackSize
                                         - sizeof(hw_stack_frame_t)
                                         - sizeof(sw_stack_frame_t));

    /* Put thread into execution */
    _sched_thread_set_exec(i, thread_def->def->tpriority);
}

/**
 * Set thread inheritance
 * Sets linking from the parent thread to the thread id.
 */
static void sched_thread_set_inheritance(pthread_t id, threadInfo_t * parent)
{
    threadInfo_t * last_node, * tmp;

    /* Initial values for all threads */
    task_table[id].inh.parent = parent;
    task_table[id].inh.first_child = NULL;
    task_table[id].inh.next_child = NULL;

    if (parent == NULL) {
#if configPROCESSSCHED != 0
        task_table[id].pid_owner = 0;
#endif
        return;
    }

#if configPROCESSSCHED != 0
        task_table[id].pid_owner = parent->pid_owner;
#endif

    if (parent->inh.first_child == NULL) {
        /* This is the first child of this parent */
        parent->inh.first_child = &(task_table[id]);
        task_table[id].inh.next_child = NULL;

        return; /* All done */
    }

    /* Find the last child thread
     * Assuming first_child is a valid thread pointer
     */
    tmp = (threadInfo_t *)(parent->inh.first_child);
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly created thread as the last child in chaini. */
    last_node->inh.next_child = &(task_table[id]);
}

void sched_thread_set_exec(int thread_id)
{
    _sched_thread_set_exec(thread_id, task_table[thread_id].def_priority);
}

/**
  * Set thread into execution mode/ready to run mode
  *
  * Sets EXEC_FLAG and puts thread into the scheduler's priority queue.
  * @param thread_id    Thread id
  * @param pri          Priority
  */
static void _sched_thread_set_exec(int thread_id, osPriority pri)
{
    /* Check that given thread is in use but not in execution */
    if (SCHED_TEST_WAKEUP_OK(task_table[thread_id].flags)) {
        task_table[thread_id].ts_counter = 4 + (int)pri;
        task_table[thread_id].priority = pri;
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
        (void)heap_insert(&priority_queue, &(task_table[thread_id]));
    }
}

void sched_thread_sleep_current(void)
{
    /* Sleep flag */
    current_thread->flags &= ~SCHED_EXEC_FLAG;

    /* Find index of the current thread in the priority queue and move it
     * on top */
    current_thread->priority = osPriorityError;
    heap_inc_key(&priority_queue, heap_find(&priority_queue, current_thread->id));
}

/**
 * Syscall was blocking.
 * Indicate that currently executing syscall is blocking and scheduler should
 * reschedule to a next thread.
 */
void sched_syscall_block(void)
{
    sched_thread_sleep_current();
    current_thread->flags |= SCHED_WAIT_FLAG;
}

/**
 * Blocking syscall is returning.
 * Reschedule a thread that was blocked by a syscall.
 * @note This function should be only called by a kworker thread.
 */
void sched_syscall_unblock(void)
{
    threadInfo_t * th;

    if (current_thread->inh.parent) {
        th = current_thread->inh.parent;

        th->flags &= ~SCHED_WAIT_FLAG;
        sched_thread_set_exec(th->id); /* Try to wakeup */
    } /* else thread was killed */
}

/**
 * Create a kworker thread that executes in privileged mode.
 * @note Immortal bit is not set by default but thread can
 *      set it by itself.
 * @param thread_def Thread defs.
 * @return thread id; Or if failed 0.
 */
pthread_t sched_create_kworker(ds_pthread_create_t * thread_def)
{
    pthread_t tt_id;

    tt_id = sched_threadCreate(thread_def);
    if (tt_id) {
        /* Just that user can see that this is a kworker, no functional
         * difference other than privileged mode. */
        task_table[tt_id].flags |= SCHED_KWORKER_FLAG;
    }

    return tt_id;
}

/**
 * Removes a thread from execution.
 * @param tt_id Thread task table id
 */
static void sched_thread_remove(pthread_t tt_id)
{
    #if configFAST_FORK != 0
    /* next_threadId_queue may break if this is not checked, otherwise it should
     * be quite safe to remove a thread multiple times. */
    if ((task_table[tt_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return;
    }
    #endif

    task_table[tt_id].flags = 0; /* Clear all flags */

    /* Release wait timeout timer */
    if (task_table[tt_id].wait_tim >= 0) {
        timers_release(task_table[tt_id].wait_tim);
    }

    /* Increment the thread priority to the highest possible value so context
     * switch will garbage collect it from the priority queue on the next run.
     */
    task_table[tt_id].priority = osPriorityError;
    {
        int i;
        i = heap_find(&priority_queue, tt_id);
        if (i != -1)
            heap_inc_key(&priority_queue, i);
    }

#if configFAST_FORK != 0
    queue_push(&next_threadId_queue_cb, &tt_id);
#endif
}

/* Functions defined in header file (and used mainly by syscalls)
 ******************************************************************************/

/**
  * @}
  */

/** @addtogroup Kernel
  * @{
  */

/** @addtogroup External_routines
  * @{
  */

/*  ==== Thread Management ==== */

pthread_t sched_threadCreate(ds_pthread_create_t * thread_def)
{
    unsigned int i;

#if configFAST_FORK != 0
    if (!queue_pop(&next_threadId_queue_cb, &i)) {
        /* New thread could not be created */
        return 0;
    }
#else
    for (i = 1; i < configSCHED_MAX_THREADS; i++) {
        if (task_table[i].flags == 0) {
#endif
            sched_thread_init(i,                 /* Index of the thread created */
                             thread_def,         /* Thread definition */
                             (void *)current_thread); /* Pointer to parent
                                                  * thread, which is expected to
                                                  * be the current thread */
#if configFAST_FORK == 0
            break;
        }
    }
#endif

    if (i == configSCHED_MAX_THREADS) {
        /* New thread could not be created */
        return 0;
    } else {
        /* Return the id of the new thread */
        return (pthread_t)i;
    }
}

pthread_t sched_thread_getId(void)
{
    return (pthread_t)(current_thread->id);
}

osStatus sched_thread_terminate(pthread_t thread_id)
{
    threadInfo_t * child;

    if (!SCHED_TEST_TERMINATE_OK(task_table[thread_id].flags)) {
        return (osStatus)osErrorParameter;
    }

    /* Remove all childs from execution */
    child = task_table[thread_id].inh.first_child;
    if (child != NULL) {
        do {
            if (sched_thread_terminate(child->id) != (osStatus)osOK) {
                child->inh.parent = 0; /* Thread is now parentles, it was
                                        * possibly a kworker that couldn't be
                                        * killed. */
            }
        } while ((child = child->inh.next_child) != NULL);
    }

    /* Remove thread itself */
    sched_thread_remove(task_table[thread_id].id);

    return (osStatus)osOK;
}

osStatus sched_thread_setPriority(pthread_t thread_id, osPriority priority)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osStatus)osErrorParameter;
    }

    /* Only thread def_priority is updated to make this syscall O(1)
     * Actual priority will be updated anyway some time later after one sleep
     * cycle.
     */
    task_table[thread_id].def_priority = priority;

    return (osStatus)osOK;
}

osPriority sched_thread_getPriority(pthread_t thread_id)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osPriority)osPriorityError;
    }

    /* Not sure if this function should return "dynamic" priority or
     * default priorty.
     */
    return task_table[thread_id].def_priority;
}


/* ==== Generic Wait Functions ==== */

osStatus sched_threadDelay(uint32_t millisec)
{
    /* osOK is always returned from delay syscall if everything went ok,
     * where as threadWait returns a pointer which may change during
     * wait time. */
    current_thread->event.status = osOK;

    if (millisec != (uint32_t)osWaitForever) {
        if ((current_thread->wait_tim = timers_add(current_thread->id, TIMERS_FLAG_ENABLED, millisec)) < 0) {
             current_thread->event.status = osErrorResource;
        }
    }

    if (current_thread->event.status != osErrorResource) {
        /* This thread shouldn't get woken up by signals */
        current_thread->flags |= SCHED_NO_SIG_FLAG;
        sched_thread_sleep_current();
    }

    return current_thread->event.status;
}

/**
  * @}
  */

/* Syscall handlers ***********************************************************/
/** @addtogroup Syscall_handlers
  * @{
  */

uintptr_t sched_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_SCHED_DELAY:
        return (uint32_t)sched_threadDelay(
                    *((uint32_t *)(p))
                );

    case SYSCALL_SCHED_GET_LOADAVG:
        sched_get_loads((uint32_t *)p);
        return (uint32_t)NULL;

    case SYSCALL_SCHED_EVENT_GET:
        *((osEvent *)(p)) = current_thread->event;
        return (uint32_t)NULL;

    default:
        return (uint32_t)NULL;
    }
}

uintptr_t sched_syscall_thread(uint32_t type, void * p)
{
    switch(type) {
    /* TODO pthread_create is allowed to throw errors and we definitely should
     *      use those. */
    case SYSCALL_SCHED_THREAD_CREATE:
        sched_threadCreate(
                    ((ds_pthread_create_t *)(p))
                );
        return 0;

    case SYSCALL_SCHED_THREAD_GETTID:
        return (uint32_t)sched_thread_getId();

    case SYSCALL_SCHED_THREAD_TERMINATE:
        return (uint32_t)sched_thread_terminate(
                    *((pthread_t *)p)
                );

    case SYSCALL_SCHED_THREAD_SETPRIORITY:
        return (uint32_t)sched_thread_setPriority(
                    ((ds_osSetPriority_t *)(p))->thread_id,
                    ((ds_osSetPriority_t *)(p))->priority
                );

    case SYSCALL_SCHED_THREAD_GETPRIORITY:
        return (uint32_t)sched_thread_getPriority(
                    *((osPriority *)(p))
                );

    case SYSCALL_SCHED_THREAD_GETERRNO:
        return (uint32_t)(current_thread->errno);

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */

/**
  * @}
  */
