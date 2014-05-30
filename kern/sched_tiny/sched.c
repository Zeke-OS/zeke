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
#include <stdint.h>
#include <stddef.h>
#include <autoconf.h>
#include <kstring.h>
#include <kinit.h>
#include <hal/core.h> /* TODO Could be removed? */
#include <klocks.h>
#include "heap.h"
#include <queue_r.h>
#include <timers.h>
#include <syscall.h>
#include <kernel.h>
#include <ptmapper.h>
#include <vralloc.h>
#include <vm/vm.h>
#include <proc.h>
#include <pthread.h>
#include <sys/sysctl.h>
#include <errno.h>
#include <kerror.h>
#include <thread.h>
#include <sched.h>

/* Definitions for load average calculation **********************************/
#define LOAD_FREQ   (configSCHED_LAVG_PER * (int)configSCHED_HZ)
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
static heap_t priority_queue = HEAP_NEW_EMPTY; /*!< Priority queue of active
                                                * threads */
/* Next thread_id queue */
static queue_cb_t next_thread_id_queue_cb;
static pthread_t next_thread_id_queue[configSCHED_MAX_THREADS - 1];

threadInfo_t * current_thread; /*!< Pointer to the currently active
                                         *   thread */
static rwlock_t loadavg_lock;
static uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/** Stack for idle task */
static char sched_idle_stack[sizeof(sw_stack_frame_t)
                             + sizeof(hw_stack_frame_t)
                             + 40];

/* sysctl node for scheduler */
SYSCTL_NODE(_kern, OID_AUTO, sched, CTLFLAG_RW, 0, "Scheduler");

/* Static init */
void sched_init(void) __attribute__((constructor));

/* Static function declarations **********************************************/
static void init_thread_id_queue(void);
void * idleTask(void * arg);
static void sched_thread_init(pthread_t i,
        struct _ds_pthread_create * thread_def, threadInfo_t * parent, int priv);
static void sched_thread_set_inheritance(threadInfo_t * new_child, threadInfo_t * parent);
static void _sched_thread_set_exec(pthread_t thread_id, osPriority pri);
static void sched_thread_remove(pthread_t id);
static void sched_thread_die(intptr_t retval);
static int sched_thread_detach(pthread_t id);
static void sched_thread_sleep(long millisec);
/* End of Static function declarations ***************************************/

/* Functions called from outside of kernel context ***************************/

/**
 * Initialize the scheduler
 */
void sched_init(void)
{
    SUBSYS_INIT();
    SUBSYS_DEP(vralloc_init);

    pthread_t tid;
    pthread_attr_t attr = {
        .tpriority  = osPriorityIdle,
        .stackAddr  = sched_idle_stack,
        .stackSize  = sizeof(sched_idle_stack)
    };
    /* Create the idle task as task 0 */
    struct _ds_pthread_create tdef_idle = {
        .thread   = &tid,
        .start    = idleTask,
        .def      = &attr,
        .argument = NULL
    };

    sched_thread_init(0, &tdef_idle, NULL, 1);
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
 * Kernel idle task
 * @note sw stacked registers are invalid when this thread executes for the
 * first time.
 */
void * idleTask(/*@unused@*/ void * arg)
{
    while(1) {
        //bcm2835_uart_uputc('I');
        idle_sleep();
    }
}

/**
 * Calculate load averages
 *
 * This function calculates unix-style load averages for the system.
 * Algorithm used here is similar to algorithm used in Linux.
 */
void sched_calc_loads(void)
{
    uint32_t active_threads; /* Fixed-point */
    static int count = LOAD_FREQ;

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
void sched_context_switcher(void)
{
    /* Select the next thread */
    do {
        /* Get next thread from the priority queue */
        current_thread = *priority_queue.a;

        if (!SCHED_TEST_CSW_OK(current_thread->flags)) {
            /* Remove the top thread from the priority queue as it is
             * either in sleep or deleted */
            (void)heap_del_max(&priority_queue);

            if (SCHED_TEST_DETACHED_ZOMBIE(current_thread->flags)) {
                sched_thread_terminate(current_thread->id);
                current_thread = 0;
            }
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

threadInfo_t * sched_get_pThreadInfo(pthread_t thread_id)
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
  * @param priv         If set thread is initialized as a kernel mode thread
  *                     (kworker).
  * @todo what if parent is stopped before this function is called?
  */
static void sched_thread_init(pthread_t i,
        struct _ds_pthread_create * thread_def, threadInfo_t * parent, int priv)
{
    /* This function should not be called for already initialized threads. */
    if ((task_table[i].flags & (uint32_t)SCHED_IN_USE_FLAG) != 0)
        return;

    memset(&(task_table[i]), 0, sizeof(threadInfo_t));

    /* Return thread id */
    if (thread_def->thread)
        *(thread_def->thread) = (pthread_t)i;

    /* Init core specific stack frame for user space */
    init_stack_frame(thread_def, &(task_table[i].sframe[SCHED_SFRAME_SYS]), priv);

    /* Mark this thread index as used.
     * EXEC flag is set later in sched_thread_set_exec */
    task_table[i].flags         = (uint32_t)SCHED_IN_USE_FLAG;
    task_table[i].id            = i;
    task_table[i].def_priority  = thread_def->def->tpriority;
    /* task_table[i].priority is set later in sched_thread_set_exec */

    if (priv) {
        /* Just that user can see that this is a kworker, no functional
         * difference other than privileged mode. */
         task_table[i].flags |= SCHED_KWORKER_FLAG;
    }

    /* Clear signal flags & wait states */
    task_table[i].wait_tim = -1;

    /* Update parent and child pointers */
    sched_thread_set_inheritance(&(task_table[i]), parent);

    /* So errno is at the last address of stack area.
     * Note that this should also agree with core specific
     * init_stack_frame() function. */
    task_table[i].errno_uaddr = (void *)((uint32_t)(thread_def->def->stackAddr)
                                        + thread_def->def->stackSize
                                        - sizeof(errno_t));

    /* Create kstack */
    thread_init_kstack(&(task_table[i]));

    /* Put thread into execution */
    _sched_thread_set_exec(i, thread_def->def->tpriority);
}

/**
 * Set thread inheritance
 * Sets linking from the parent thread to the thread id.
 */
static void sched_thread_set_inheritance(threadInfo_t * new_child, threadInfo_t * parent)
{
    threadInfo_t * last_node, * tmp;

    /* Initial values for all threads */
    new_child->inh.parent = parent;
    new_child->inh.first_child = NULL;
    new_child->inh.next_child = NULL;

    if (parent == NULL) {
        new_child->pid_owner = 0;
        return;
    }
    new_child->pid_owner = parent->pid_owner;

    if (parent->inh.first_child == NULL) {
        /* This is the first child of this parent */
        parent->inh.first_child = new_child;
        new_child->inh.next_child = NULL;

        return; /* All done */
    }

    /* Find the last child thread
     * Assuming first_child is a valid thread pointer
     */
    tmp = (threadInfo_t *)(parent->inh.first_child);
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly created thread as the last child in chain. */
    last_node->inh.next_child = new_child;
}

pthread_t sched_thread_fork(void)
{
    threadInfo_t * const old_thread = current_thread;
    threadInfo_t tmp;
    pthread_t new_id;

#if configDEBUG >= KERROR_DEBUG
    if (old_thread == 0) {
        panic("current_thread not set");
    }
#endif

    if (!queue_pop(&next_thread_id_queue_cb, &new_id)) {
        return -ENOMEM;
    }

    /* New thread is kept in tmp until it's ready for execution. */
    memcpy(&tmp, old_thread, sizeof(threadInfo_t));
    tmp.flags &= ~SCHED_EXEC_FLAG; /* Disable exec for now. */
    tmp.id = new_id;
    sched_thread_set_inheritance(&tmp, old_thread);

    /* Initialize a new kstack & copy data from old kstack. */
    thread_init_kstack(&tmp);
#if 0
    memcpy((void *)(tmp.kstack_region->mmu.paddr),
            (void *)(old_thread->kstack_region->mmu.paddr),
            MMU_SIZEOF_REGION(&(old_thread->kstack_region->mmu)));
#endif

    memcpy(&tmp.sframe[SCHED_SFRAME_SYS], &old_thread->sframe[SCHED_SFRAME_SVC],
            sizeof(sw_stack_frame_t));
    tmp.sframe[SCHED_SFRAME_SYS].r0 = 0;
    tmp.sframe[SCHED_SFRAME_SYS].pc += 4; /* TODO This is too hw specific */
    memcpy(&(task_table[new_id]), &tmp, sizeof(threadInfo_t));

    /* TODO Increment resource refcounters(?) */

    return new_id;
}

void sched_thread_set_exec(pthread_t thread_id)
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
static void _sched_thread_set_exec(pthread_t thread_id, osPriority pri)
{
    istate_t s;

    /* Check that given thread is in use but not in execution */
    if (SCHED_TEST_WAKEUP_OK(task_table[thread_id].flags)) {
        s = get_interrupt_state();
        disable_interrupt(); /* TODO Not MP safe! */

        task_table[thread_id].ts_counter = 4 + (int)pri;
        task_table[thread_id].priority = pri;
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
        (void)heap_insert(&priority_queue, &(task_table[thread_id]));

        set_interrupt_state(s);
    }
}

void sched_thread_sleep_current(void)
{
    istate_t s;

    s = get_interrupt_state();
    disable_interrupt(); /* TODO Not MP safe! */

    /* Sleep flag */
    current_thread->flags &= ~SCHED_EXEC_FLAG;

    /* Find index of the current thread in the priority queue and move it
     * on top */
    current_thread->priority = osPriorityError;

    heap_inc_key(&priority_queue, heap_find(&priority_queue, current_thread->id));

    set_interrupt_state(s);
}

/**
 * Removes a thread from scheduling.
 * @param tt_id Thread task table id
 */
static void sched_thread_remove(pthread_t tt_id)
{
    istate_t s;

    if ((task_table[tt_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return;
    }

    /* Notify the owner about removal of a thread. */
    if (task_table[tt_id].pid_owner != 0) {
        proc_thread_removed(task_table[tt_id].pid_owner, tt_id);
    }

    s = get_interrupt_state();
    disable_interrupt();

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

    /* Release thread id */
    queue_push(&next_thread_id_queue_cb, &tt_id);

    set_interrupt_state(s);
}

/**
 * Terminate current thread.
 * This makes current_thread a zombie that should be either killed by the
 * parent thread or will be killed at least when the parent is killed.
 * @param retval is a return value from the thread.
 */
static void sched_thread_die(intptr_t retval)
{
    current_thread->flags |= SCHED_ZOMBIE_FLAG;
    sched_thread_sleep_current();
    current_thread->retval = retval;
    /* Thread will now block and next thread will be scheduled in. */
}

/**
 * Mark thread as detached so it wont be turned into zombie on exit.
 * @param id is a thread id.
 * @return Return -EINVAL if invalid thread id was given; Otherwise zero.
 */
static int sched_thread_detach(pthread_t id)
{
    if ((task_table[id].flags & SCHED_IN_USE_FLAG) == 0) {
        return -EINVAL;
    }

    task_table[id].flags |= SCHED_DETACH_FLAG;

    if (SCHED_TEST_DETACHED_ZOMBIE(task_table[id].flags)) {
        /* Workaround to remove the thread without locking the system now
         * because we don't want to disable interrupts here for a long tome
         * and we have no other proctection in the scheduler right now. */
        istate_t s;
        int i;

        /* TODO This part is not quite clever nor MP safe. */
        s = get_interrupt_state();
        disable_interrupt();

        i = heap_find(&priority_queue, id);

        if (i < 0)
            (void)heap_insert(&priority_queue, &(task_table[id]));
        /* It will be now definitely gc'ed at some point. */
        set_interrupt_state(s);
    }

    return 0;
}

static void sched_thread_sleep(long millisec)
{
    while ((current_thread->wait_tim = timers_add(current_thread->id,
                    TIMERS_FLAG_ENABLED, millisec)) < 0);
    sched_thread_sleep_current();
    idle_sleep();
    while (current_thread->wait_tim >= 0); /* We may want to do someting in this
                                            * loop if thread is woken up but
                                            * timer is not released. */
}

/* Functions defined in header file
 ******************************************************************************/

/*  ==== Thread Management ==== */

pthread_t sched_threadCreate(struct _ds_pthread_create * thread_def, int priv)
{
    pthread_t i;
#if 0
    istate_t s;

    s = get_interrupt_state();
    disable_interrupt();
#endif

    if (!queue_pop(&next_thread_id_queue_cb, &i)) {
        /* New thread could not be created */
#if 0
        set_interrupt_state(s);
#endif
        return -1;
    }

    sched_thread_init(
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

/* TODO Might be unsafe to call from multiple threads for the same tree! */
int sched_thread_terminate(pthread_t thread_id)
{
    threadInfo_t * child;
    threadInfo_t * prev_child;

    if (!SCHED_TEST_TERMINATE_OK(task_table[thread_id].flags)) {
        return -EPERM;
    }

    /* Remove all child threads from execution */
    child = task_table[thread_id].inh.first_child;
    prev_child = NULL;
    if (child != NULL) {
        do {
            if (sched_thread_terminate(child->id) != 0) {
                child->inh.parent = 0; /* Thread is now parentles, it was
                                        * possibly a kworker that couldn't be
                                        * killed. */
            }

            /* Fix child list */
            if (child->flags &&
                    (((threadInfo_t *)(task_table[thread_id].inh.first_child))->flags == 0)) {
                task_table[thread_id].inh.first_child = child;
                prev_child = child;
            } else if (child->flags && prev_child) {
                prev_child->inh.next_child = child;
                prev_child = child;
            } else if (child->flags) {
                prev_child = child;
            }
        } while ((child = child->inh.next_child) != NULL);
    }

    /* We set this thread as a zombie. If detach is also set then
     * sched_thread_remove() will remove this thread immediately but usually
     * it's not, then it will release some resourses but left the thread
     * struct mostly intact. */
    task_table[thread_id].flags |= SCHED_ZOMBIE_FLAG;
    task_table[thread_id].flags &= ~SCHED_EXEC_FLAG;

    /* Remove thread completely if it is a detached zombie, its parent is a
     * detached zombie thread or the thread is parentles for any reason.
     * Otherwise we left the thread struct intact for now.
     */
    if (SCHED_TEST_DETACHED_ZOMBIE(task_table[thread_id].flags) ||
            (task_table[thread_id].inh.parent == 0)             ||
            ((task_table[thread_id].inh.parent != 0) &&
            SCHED_TEST_DETACHED_ZOMBIE(((threadInfo_t *)(task_table[thread_id].inh.parent))->flags))) {
        sched_thread_remove(task_table[thread_id].id);
    }

    return 0;
}

int sched_thread_set_priority(pthread_t thread_id, osPriority priority)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return -ESRCH;
    }

    /* Only thread def_priority is updated to make this syscall O(1)
     * Actual priority will be updated anyway some time later after one sleep
     * cycle.
     */
    task_table[thread_id].def_priority = priority;

    return 0;
}

osPriority sched_thread_get_priority(pthread_t thread_id)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osPriority)osPriorityError;
    }

    /* Not sure if this function should return "dynamic" priority or
     * default priorty.
     */
    return task_table[thread_id].def_priority;
}

/* Syscall handlers ***********************************************************/

uintptr_t sched_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_SCHED_DIE:
        /* We don't care about validity of a possible pointer returned as a
         * return value because we don't touch it in the kernel. */
        sched_thread_die((intptr_t)p); /* Thread will eventually block now... */
        return 0;

    case SYSCALL_SCHED_DETACH:
        {
        pthread_t thread_id;
        if (!useracc(p, sizeof(pthread_t), VM_PROT_READ)) {
            /* No permission to read */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
            return -1;
        }
        copyin(p, &thread_id, sizeof(pthread_t));
        if ((uintptr_t)sched_thread_detach(thread_id)) {
            set_errno(EINVAL);
            return -1;
        }
        return 0;
        }

    case SYSCALL_SCHED_SETPRIORITY:
        {
        int err;
        struct _ds_set_priority ds;
        if (!useracc(p, sizeof(struct _ds_set_priority), VM_PROT_READ)) {
            set_errno(ESRCH);
            return -1;
        }
        copyin(p, &ds, sizeof(struct _ds_set_priority));
        err = (uintptr_t)sched_thread_set_priority(ds.thread_id, ds.priority);
        if (err) {
            set_errno(-err);
            return -1;
        }
        return 0;
        }

    case SYSCALL_SCHED_GETPRIORITY:
        {
        osPriority pri;
        pthread_t thread_id;
        if (!useracc(p, sizeof(pthread_t), VM_PROT_READ)) {
            /* No permission to read */
            /* TODO Signal/Kill? */
            set_errno(ESRCH);
            return -1;
        }
        copyin(p, &thread_id, sizeof(pthread_t));
        pri = (uintptr_t)sched_thread_get_priority(thread_id);
        if (pri == osPriorityError) {
            set_errno(ESRCH);
            pri = -1; /* Note: -1 might be also legitimate prio value. */
        }
        return pri;
        }

    case SYSCALL_SCHED_SLEEP_MS:
        {
        uint32_t val;
        if (!useracc(p, sizeof(uint32_t), VM_PROT_READ)) {
            /* No permission to read/write */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
            return -EFAULT;
        }
        copyin(p, &val, sizeof(uint32_t));
        sched_thread_sleep(val);
        return 0; /* TODO Return value might be incorrect */
        }

    case SYSCALL_SCHED_GET_LOADAVG:
        {
        uint32_t arr[3];
        if (!useracc(p, sizeof(arr), VM_PROT_WRITE)) {
            /* No permission to write */
            /* TODO Signal/Kill? */
            set_errno(EFAULT);
            return -1;
        }
        sched_get_loads(arr);
        copyout(arr, p, sizeof(arr));
        }
        return (uintptr_t)NULL;

    default:
        set_errno(ENOSYS);
        return (uintptr_t)NULL;
    }
}
