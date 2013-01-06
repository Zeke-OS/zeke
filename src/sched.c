/**
 *******************************************************************************
 * @file    sched.c
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler
 *******************************************************************************
 */

/** @addtogroup Scheduler
  * @{
  */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "thread.h"

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include "heap.h"
#include "timers.h"
#include "hal_core.h"
#include "kernel.h"
#include "sched.h"

/* Definitions for load average calculation **********************************/
#define LOAD_FREQ   (configSCHED_LAVG_PER * configSCHED_FREQ)
/* FEXP_N = 2^11/(2^(interval * log_2(e/N))) */
#if configSCHED_LAVG_PER == 5
#define FSHIFT      11      /* nr of bits of precision */
#define FEXP_1      1884    /* 1/exp(5sec/1min) */
#define FEXP_5      2014    /* 1/exp(5sec/5min) */
#define FEXP_15     2037    /* 1/exp(5sec/15min) */
#elif configSCHED_LAVG_PER == 11
#define FSHIFT      11
#define FEXP_1      1704
#define FEXP_5      1974
#define FEXP_15     2023
#else
#error Incorrect value of kernel configuration configSCHED_LAVG_PER
#endif
#define FIXED_1     (1 << FSHIFT)
#define CALC_LOAD(load, exp, n)                  \
                    load *= exp;                 \
                    load += n * (FIXED_1 - exp); \
                    load >>= FSHIFT;
/** Scales fixed-point load average value to a integer format scaled to 100 */
#define SCALE_LOAD(x) ((x + (FIXED_1/200) * 100) >> FSHIFT)
/* End of Definitions for load average calculation ***************************/

volatile uint32_t sched_enabled = 0; /* If this is set to != 0 interrupt
                                      * handlers will be able to call context
                                      * switching. */

/* Task containers */
threadInfo_t task_table[configSCHED_MAX_THREADS]; /*!< Array of all threads */
static heap_t priority_queue = HEAP_NEW_EMPTY; /*!< Priority queue of active
                                                * threads */

volatile threadInfo_t * current_thread; /*!< Pointer to currently active thread */
uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/* Stack for idle task */
static char sched_idle_stack[sizeof(sw_stack_frame_t) + sizeof(hw_stack_frame_t) + 100];
int _first_switch = 1;

/* Static function declarations **********************************************/
void idleTask(void * arg);
static void calc_loads(void);
static void context_switcher(void);
static void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent);
static void sched_thread_set_inheritance(osThreadId i, threadInfo_t * parent);
static void _sched_thread_set_exec(int thread_id, osPriority pri);
static void _sched_thread_sleep_current(void);
static void sched_thread_remove(uint32_t id);
/* End of Static function declarations ***************************************/

/* Functions called from outside of kernel context ***************************/

/**
  * Initialize the scheduler
  */
void sched_init(void)
{
    /* Create the idle task as task 0 */
    osThreadDef_t tdef_idle = { (os_pthread)(&idleTask),
                                osPriorityIdle,
                                sched_idle_stack,
                                sizeof(sched_idle_stack) / sizeof(char)
                              };
    sched_thread_set(0, &tdef_idle, NULL, NULL);

    /* Set idle thread as currently running thread */
    current_thread = &(task_table[0]);

    /* Set initial value for PSP */
    wr_thread_stack_ptr(task_table[0].sp);
}

/**
  * Start scheduler
  */
void sched_start(void)
{
    __disable_interrupt();
    sched_enabled = 1;
    __enable_interrupt(); /* Enable interrupts */
}

/* End of Functions called from outside of kernel context ********************/

/**
 * Kernel idle task
 * @todo Add support for low power modes when idling
 */
void idleTask(void * arg)
{
#ifndef PU_TEST_BUILD
    while(1);
#endif
}

#ifndef PU_TEST_BUILD
/**
  * Scheduler handler
  *
  * Scheduler handler is mainly called by sysTick and PendSV.
  */
void sched_handler(void)
{
    if (!_first_switch) {
        save_context();
    } else _first_switch = 0;
    current_thread->sp = (void *)rd_thread_stack_ptr();

    /* - Tasks before context switch - */
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) { /* Run only if systick */
        calc_loads();
        timers_run();
    }
    /* End of tasks */

    context_switcher();
    load_context(); /* Since PSP has been updated, this loads the last state of
                     * the new task */
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
        active_threads = priority_queue.size * FIXED_1;

        /* Load averages */
        CALC_LOAD(loadavg[0], FEXP_1, active_threads);
        CALC_LOAD(loadavg[1], FEXP_5, active_threads);
        CALC_LOAD(loadavg[2], FEXP_15, active_threads);
    }
}

/**
 * Return load averages in integer format scaled to 100.
 */
void sched_get_loads(uint32_t * loads)
{
    loads[0] = SCALE_LOAD(loadavg[0]);
    loads[1] = SCALE_LOAD(loadavg[1]);
    loads[2] = SCALE_LOAD(loadavg[2]);
}

static void context_switcher(void)
{
    /* Select next thread */
    do {
        /* We can only do some operations efficiently if the current thread is
         * on the top of the heap. If some other thread has been added on the
         * top while running the current thread it doesn't matter as sleeping
         * or dead threads will be will be removed when there is no other
         * threads prioritized above them. This actually means that garbage
         * collection respects thread priorities and it  doesn't cause that
         * much overhead for higher priority threads. */
        if (current_thread == priority_queue.a[0]) {
            /* Scaled maximum time slice count */
            int tslice_n_max;
            tslice_n_max = configSCHED_MAX_SLICES;
            if ((int)current_thread->priority >= 0) { /* Positive priority */
                tslice_n_max *= ((int)current_thread->priority + 1);
            } else { /* Negative priority */
                tslice_n_max /= (-(int)current_thread->priority + 1);
            }

            if ((current_thread->flags & SCHED_EXEC_FLAG) == 0) {
                /* Remove the current thread from the priority queue this will
                 * also GC dead threads */
                (void)heap_del_max(&priority_queue);
                if ((current_thread->flags & SCHED_IN_USE_FLAG) == 0) {
                    /* Thread is in sleep so revert back to its original
                     * priority */
                    current_thread->priority = current_thread->def_priority;
                    current_thread->uCounter = 0;
                } /* else thread is also deleted and no further action is
                   * needed */
            } else if ((current_thread->uCounter >= (uint32_t)tslice_n_max)
                        /* if maximum slices for this thread */
              && ((int)current_thread->priority > (int)osPriorityLow))
                        /* if priority is higher than this */
            {
                /* Give a penalty: Set lower priority and
                 * perform heap decrement operation. */
                current_thread->priority = osPriorityLow;
                heap_dec_key(&priority_queue, 0);

                /* WARNING: Starvation is still possible for other osPriorityLow
                 *          threads. */
            }
        }

        /* Next thread is the top one on the priority queue */
        current_thread = *priority_queue.a;

        /* In the do...while loop condition we'll skip this thread
         * if its EXEC or IN_USE flags are disabled. */
    } while ((current_thread->flags & SCHED_CSW_OK_FLAGS) != SCHED_CSW_OK_FLAGS);

    /* uCounter is used to determine how many time slices has been used
     * by the process. */
    current_thread->uCounter++;

    /* Write the value of the PSP for the next thread in run state */
    wr_thread_stack_ptr(current_thread->sp);
}

/**
  * Set thread initial configuration
  * @note This function should not be called for already initialized threads.
  * @param i            Thread id
  * @param thread_def   Thread definitions
  * @param argument     Thread argument
  * @param parent       Parent thread id, NULL = doesn't have a parent
  * @todo what if parent is stopped before we call this?
  */
static void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent)
{
    /* This function should not be called for already initialized threads. */
    if ((task_table[i].flags & SCHED_IN_USE_FLAG) != 0)
        return;

    /* Init core specific stack frame */
    init_hw_stack_frame(thread_def, argument, (uint32_t)del_thread);

    /* Mark that this thread position is in use.
     * EXEC flag is set later in sched_thread_set_exec */
    task_table[i].flags         = SCHED_IN_USE_FLAG;
    task_table[i].id            = i;
    task_table[i].def_priority  = thread_def->tpriority;
    /* task_table[i].priority is set later in sched_thread_set_exec */

    /* Clear signal flags & wait states */
    task_table[i].signals = 0;
    task_table[i].sig_wait_mask = 0x0;

    /* Clear events */
    memset(&(task_table[i].event), 0, sizeof(osEvent));

    /* Update parent and child pointers */
    sched_thread_set_inheritance(i, parent);

    /* Update stack pointer */
    task_table[i].sp = (void *)((uint32_t)(thread_def->stackAddr)
                                         + thread_def->stackSize
                                         - sizeof(hw_stack_frame_t)
                                         - sizeof(sw_stack_frame_t));

    /* Put thread into execution */
    _sched_thread_set_exec(i, thread_def->tpriority);
}

static void sched_thread_set_inheritance(osThreadId i, threadInfo_t * parent)
{
    threadInfo_t * last_node, * tmp;

    /* Initial values for all threads */
    task_table[i].inh.parent = parent;
    task_table[i].inh.first_child = NULL;
    task_table[i].inh.next_child = NULL;

    if (parent == NULL)
        return;

    if (parent->inh.first_child == NULL) {
        /* This is the first child of this parent */
        parent->inh.first_child = &(task_table[i]);
        task_table[i].inh.next_child = NULL;

        return; /* All done */
    }

    /* Find last child thread
     * Assuming first_child is a valid thread pointer
     */
    tmp = (threadInfo_t *)(parent->inh.first_child);
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly created thread as a last child */
    last_node->inh.next_child = &(task_table[i]);
}

void sched_thread_set_exec(int thread_id)
{
    _sched_thread_set_exec(thread_id, task_table[thread_id].def_priority);
}

/**
  * Set thread into excetion mode
  *
  * Sets EXEC_FLAG and puts thread into the scheduler's priority queue.
  * @param thread_id    Thread id
  * @param pri          Priority
  */
static void _sched_thread_set_exec(int thread_id, osPriority pri)
{
    /* Check that given thread is in use but not in execution */
    if ((task_table[thread_id].flags & (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG)) == SCHED_IN_USE_FLAG) {
        task_table[thread_id].uCounter = 0;
        task_table[thread_id].priority = pri;
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
        (void)heap_insert(&priority_queue, &(task_table[thread_id]));
    }
}

static void _sched_thread_sleep_current(void)
{
    int i = 0;

    /* Sleep flag */
    current_thread->flags &= ~SCHED_EXEC_FLAG;

    /* Find index of the current thread in the priority queue and move it
     * on top */
    for (i = 0; i < priority_queue.size; i++) {
        if (priority_queue.a[i]->id == current_thread->id)
            break;
    }
    current_thread->priority = osPriorityError;
    heap_inc_key(&priority_queue, i);
}

/**
 * Removes thread from execution
 * @param tt_id Thread task table id
 */
static void sched_thread_remove(uint32_t tt_id)
{
    int i;

    task_table[tt_id].flags = 0; /* Clear all the flags */

    /* Find thread from the priority queue and put it on top.
     * This way we can be sure that this thread is not kept in the queue after
     * context switch is executed. If priority is not incremented it's possible
     * that we would fill the queue with garbage. */
    for (i = 0; i < priority_queue.size; i++) {
        if (priority_queue.a[i]->id == tt_id)
            break;
    }

    task_table[tt_id].priority = osPriorityError;
    heap_inc_key(&priority_queue, i);
}


/* Functions defined in header file (and used mainly by syscalls)
 ******************************************************************************/

/*  ==== Thread Management ==== */

/**
  * Create a new thread
  *
  */
osThreadId sched_ThreadCreate(osThreadDef_t * thread_def, void * argument)
{
    int i;

    /* Disable context switching to support multi-threaded calls to this fn */
    __istate_t s = __get_interrupt_state();
    __disable_interrupt();

    for (i = 1; i < configSCHED_MAX_THREADS; i++) {
        if (task_table[i].flags == 0) {
            sched_thread_set(i,                  /* Index of created thread */
                             thread_def,         /* Thread definition */
                             argument,           /* Argument */
                             (void *)current_thread); /* Pointer to parent
                                                  * thread, which is expected to
                                                  * be the current thread */
            break;
        }
    }
    __set_interrupt_state(s); /* Restore interrupts */

    if (i == configSCHED_MAX_THREADS) {
        /* New thread could not be created */
        return 0;
    } else {
        /* Return the id of the new thread */
        return (osThreadId)i;
    }
}

/**
 * Get id of currently running thread
 * @return Thread id of current thread
 */
osThreadId sched_thread_getId(void)
{
    return (osThreadId)(current_thread->id);
}

osStatus sched_thread_terminate(osThreadId thread_id)
{
    threadInfo_t * child;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osStatus)osErrorParameter;
    }

    /* Remove all childs from execution */
    child = task_table[thread_id].inh.first_child;
    do {
        sched_thread_remove(child->id);
    } while ((child = child->inh.next_child) != NULL);

    /* Remove thread itself */
    sched_thread_remove(task_table[thread_id].id);

    return (osStatus)osOK;
}

/**
 * Set thread priority
 * @param thread_id Thread id
 * @param priority New priority for thread referenced by thread_id
 * @return osOK if thread exists
 */
osStatus sched_thread_setPriority(osThreadId thread_id, osPriority priority)
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

osPriority sched_thread_getPriority(osThreadId thread_id)
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

    if (millisec != osWaitForever) {
        if (timers_add(current_thread->id, millisec) != 0) {
             current_thread->event.status = osErrorResource;
        }
    }

    if (current_thread->event.status != osErrorResource) {
        /* This thread shouldn't get woken up by signals */
        current_thread->flags |= SCHED_NO_SIG_FLAG;
        _sched_thread_sleep_current();
    }

    return current_thread->event.status;
}

/**
 * Thread wait syscall handler
 * millisec Event wait time in ms or osWaitForever.
 * @note This function returns a pointer to a thread event struct and its
 * contents is allowed to change before returning back to the caller thread.
 */
osEvent * sched_threadWait(uint32_t millisec)
{
    return sched_threadSignalWait(0x7fffffff, millisec);
}

/* ==== Signal Management ==== */

int32_t sched_threadSignalSet(osThreadId thread_id, int32_t signal)
{
    int32_t prev_signals;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000; /* Error code specified in CMSIS-RTOS */

    prev_signals = (uint32_t)task_table[thread_id].signals;

    task_table[thread_id].signals |= signal; /* OR with all signals */
    task_table[thread_id].event.value.signals = signal; /* Only this signal as
                                             * the event was calling of this */
    task_table[thread_id].event.status = osEventSignal;

    if (((task_table[thread_id].flags & SCHED_NO_SIG_FLAG) == 0)
        && ((task_table[thread_id].sig_wait_mask & signal) != 0)) {
        /* Clear signal wait mask */
        task_table[thread_id].sig_wait_mask = 0x0;

        /* Set the signaled thread back into execution */
        _sched_thread_set_exec(thread_id, task_table[thread_id].def_priority);
    }

    return prev_signals;
}

/**
 * Clear signal wait mask of the current thread
 */
void sched_threadSignalWaitMaskClear(void)
{
    current_thread->sig_wait_mask = 0x0;
}

int32_t sched_threadSignalClear(osThreadId thread_id, int32_t signal)
{
    int32_t prev_signals;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000; /* Error code specified in CMSIS-RTOS */

    prev_signals = (uint32_t)task_table[thread_id].signals;
    task_table[thread_id].signals &= ~(signal & 0x7fffffff);

    return prev_signals;
}

int32_t sched_threadSignalGetCurrent(void)
{
    return current_thread->signals;
}

int32_t sched_threadSignalGet(osThreadId thread_id)
{
    return task_table[thread_id].signals;
}

osEvent * sched_threadSignalWait(int32_t signals, uint32_t millisec)
{
    current_thread->event.status = osEventTimeout;

    if (millisec != osWaitForever) {
        if (timers_add(current_thread->id, millisec) != 0) {
            /* Timer error will be most likely but not necessarily returned
             * as is. There is however a slight chance of event triggering
             * before control returns back to this thread. It is completely OK
             * to clear this error if that happens. */
            current_thread->event.status = osErrorResource;
        }
    }

    if (current_thread->event.status != osErrorResource) {
        current_thread->sig_wait_mask = signals;
        _sched_thread_sleep_current();
    }

    /* Event status is now timeout but will be changed if any event occurs
     * as event is returned as a pointer. */
    return (osEvent *)(&(current_thread->event));
}
