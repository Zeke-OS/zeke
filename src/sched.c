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
#include "hal_mcu.h"
#include "kernel.h"
#include "sched.h"

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

volatile uint32_t sched_enabled = 0; /* If this is set to != 0 interrupt
                                      * handlers will be able to call context
                                      * switching. */

/* Task containers */
static threadInfo_t task_table[configSCHED_MAX_THREADS]; /*!< Array of all
                                                          *   threads */
static heap_t priority_queue = HEAP_NEW_EMPTY;   /*!< Priority queue of active
                                                  * threads */

static volatile threadInfo_t * current_thread; /*!< Pointer to currently active
                                                *   thread */
uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/** Stack for idle task */
static char sched_idle_stack[sizeof(sw_stack_frame_t)
                             + sizeof(hw_stack_frame_t)
                             + 40]; /* Absolute minimum with current idle task
                                     * implementation */

/* Static function declarations **********************************************/
void idleTask(void * arg);
static void calc_loads(void);
static void context_switcher(void);
static void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent);
static void sched_thread_set_inheritance(osThreadId i, threadInfo_t * parent);
static void _sched_thread_set_exec(int thread_id, osPriority pri);
static void _sched_thread_sleep_current(void);
static void sched_thread_remove(osThreadId id);
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

    /* sw_stack_frame will be overwritten when scheduler is run for the first
     * time. This also means that sw stacked registers are invalid when
     * idle task executes for the first time. */
    current_thread->sp = (uint32_t *)((uint32_t)(current_thread->sp)
                                          + sizeof(sw_stack_frame_t));

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
 * @note sw stacked registers are invalid when this thread executes for the
 * first time.
 * @todo Add support for low power modes when idling
 */
void idleTask(/*@unused@*/ void * arg)
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
    save_context(); /* Registers should remain untouched before this point */
    current_thread->sp = (void *)rd_thread_stack_ptr();

    eval_kernel_tick();
    if (flag_kernel_tick) { /* Run only if tick was set */
        timers_run();
    }

    context_switcher();

    if (flag_kernel_tick) {
        calc_loads();
    }
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

/**
 * Return load averages in integer format scaled to 100.
 */
void sched_get_loads(uint32_t loads[3])
{
    loads[0] = SCALE_LOAD(loadavg[0]);
    loads[1] = SCALE_LOAD(loadavg[1]);
    loads[2] = SCALE_LOAD(loadavg[2]);
}

static void context_switcher(void)
{
    /* Select next thread */
    do {
        /* Get next thread from the priority queue */
        current_thread = *priority_queue.a;

        if (   ((current_thread->flags & SCHED_EXEC_FLAG) == 0)
            || ((current_thread->flags & SCHED_IN_USE_FLAG) == 0)) {
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
            /* Give a penalty: Set lower priority
             * and perform heap decrement operation. */
            current_thread->priority = osPriorityLow;
            heap_dec_key(&priority_queue, 0);

            /* WARNING: Starvation is still possible if there is other threads
             * with priority set to osPriorityLow.
             */
            continue; /* Select next thread */
        }

        /* Thread is skipped if its EXEC or IN_USE flags are disabled. */
    } while ((current_thread->flags & SCHED_CSW_OK_FLAGS) != SCHED_CSW_OK_FLAGS);

    /* ts_counter is used to determine how many time slices has been used by the
     * process between idle/sleep states. We can assume that this measure is
     * quite accurate even it's not confirmed that one tick has been elapsed
     * before this line. */
    current_thread->ts_counter--;

    /* Write the value of the PSP for the next thread in exec */
    wr_thread_stack_ptr(current_thread->sp);
}

/**
  * Set thread initial configuration
  * @note This function should not be called for already initialized threads.
  * @param i            Thread id
  * @param thread_def   Thread definitions
  * @param argument     Thread argument
  * @param parent       Parent thread id, NULL = doesn't have a parent
  * @todo what if parent is stopped before this function is called?
  */
static void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent)
{
    /* This function should not be called for already initialized threads. */
    if ((task_table[i].flags & (uint32_t)SCHED_IN_USE_FLAG) != 0)
        return;

    /* Init core specific stack frame */
    init_hw_stack_frame(thread_def, argument, (uint32_t)del_thread);

    /* Mark that this thread position is in use.
     * EXEC flag is set later in sched_thread_set_exec */
    task_table[i].flags         = (uint32_t)SCHED_IN_USE_FLAG;
    task_table[i].id            = i;
    task_table[i].def_priority  = thread_def->tpriority;
    /* task_table[i].priority is set later in sched_thread_set_exec */

    /* Clear signal flags & wait states */
    task_table[i].signals = 0;
    task_table[i].sig_wait_mask = 0x0;
    task_table[i].wait_tim = -1;

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

/**
 * Set thread into execution mode/ready to run mode.
 * @oaram thread_id Thread id.
 */
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
    if ((task_table[thread_id].flags & (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG))
        == SCHED_IN_USE_FLAG) {
        task_table[thread_id].ts_counter = 4 + (int)pri;
        task_table[thread_id].priority = pri;
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
        (void)heap_insert(&priority_queue, &(task_table[thread_id]));
    }
}

/**
 * Put current thread into sleep.
 */
static void _sched_thread_sleep_current(void)
{
    /* Sleep flag */
    current_thread->flags &= ~SCHED_EXEC_FLAG;

    /* Find index of the current thread in the priority queue and move it
     * on top */
    current_thread->priority = osPriorityError;
    heap_inc_key(&priority_queue, heap_find(&priority_queue, current_thread->id));
}

/**
 * Removes a thread from execution.
 * @param tt_id Thread task table id
 */
static void sched_thread_remove(osThreadId tt_id)
{
    task_table[tt_id].flags = 0; /* Clear all flags */

    /* Release wait timeout timer */
    if (task_table[tt_id].wait_tim >= 0) {
        timers_release(task_table[tt_id].wait_tim);
    }

    /* Increment the thread priority to the highest possible value so context
     * switch will garbage collect it from the priority queue on the next run.
     */
    task_table[tt_id].priority = osPriorityError;
    heap_inc_key(&priority_queue, heap_find(&priority_queue, tt_id));
}


/* Functions defined in header file (and used mainly by syscalls)
 ******************************************************************************/

/** @addtogroup External_services
  * @{
  */

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
 * @param   thread_id Thread id
 * @param   priority New priority for thread referenced by thread_id
 * @return  osOK if thread exists
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

    if (millisec != (uint32_t)osWaitForever) {
        if (timers_add(current_thread->id, osTimerOnce, millisec) < 0) {
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
 * @param millisec Event wait time in ms or osWaitForever.
 * @note This function returns a pointer to a thread event struct and its
 * contents is allowed to change before returning back to the caller thread.
 */
osEvent * sched_threadWait(uint32_t millisec)
{
    return sched_threadSignalWait(0x7fffffff, millisec);
}

/* ==== Signal Management ==== */

/**
 * Set signal and wakeup the thread.
 */
int32_t sched_threadSignalSet(osThreadId thread_id, int32_t signal)
{
    int32_t prev_signals;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000; /* Error code specified in CMSIS-RTOS */

    prev_signals = task_table[thread_id].signals;

    task_table[thread_id].signals |= signal; /* OR with all signals */

    /* Update event struct */
    task_table[thread_id].event.value.signals = signal; /* Only this signal */
    task_table[thread_id].event.status = osEventSignal;

    if (((task_table[thread_id].flags & SCHED_NO_SIG_FLAG) == 0)
        && ((task_table[thread_id].sig_wait_mask & signal) != 0)) {
        /* Release wait timeout timer */
        if (task_table[thread_id].wait_tim >= 0) {
            timers_release(task_table[thread_id].wait_tim);
        }

        /* Clear signal wait mask */
        task_table[thread_id].sig_wait_mask = 0x0;

        /* Set the signaled thread back into execution */
        _sched_thread_set_exec(thread_id, task_table[thread_id].def_priority);
    }

    return prev_signals;
}

#if configDEVSUBSYS == 1
/**
 * Send a signal that a dev resource is free now.
 * @param signal signal mask
 */
void sched_threadDevSignal(int32_t signal, osDev_t dev)
{
    int i;
    unsigned int temp_dev = DEV_MAJOR(dev);

    /* This is unfortunately O(n) :'(
     * TODO Some prioritizing would be very nice at least.
     *      Possibly if we would just add waiting threads first to a
     *      priority queue? Is it a waste of cpu time? It isn't actually
     *      a quaranteed way to remove starvation so starvation would be
     *      still there :/
     */
    i = 0;
    do {
        if (   ((task_table[i].sig_wait_mask & signal)    != 0)
            && ((task_table[i].flags & SCHED_IN_USE_FLAG) != 0)
            && ((task_table[i].flags & SCHED_NO_SIG_FLAG) == 0)
            && ((task_table[i].dev_wait) == temp_dev)) {
            task_table[i].dev_wait = 0u;
            /* I feel this is bit wrong but we won't save and return
             * prev_signals since no one cares... */
            sched_threadSignalSet(task_table[i].id, signal);
            /* We also assume that the signaling was succeed, if it wasn't we
             * are in deep trouble. But it will never happen!
             */

            return; /* Return now so other threads will keep waiting for their
                     * turn. */
        }
    } while (++i < configSCHED_MAX_THREADS);

    return; /* Thre is no threads waiting for this signal,
             * so we wasted a lot of cpu time.             */
}
#endif

/**
 * Clear signal wait mask of the current thread
 */
void sched_threadSignalWaitMaskClear(void)
{
    current_thread->sig_wait_mask = 0x0;
}

/**
 * Clear selected signals
 * @param thread_id Thread id
 * @param signal    Signals to be cleared.
 */
int32_t sched_threadSignalClear(osThreadId thread_id, int32_t signal)
{
    int32_t prev_signals;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000; /* Error code specified in CMSIS-RTOS */

    prev_signals = task_table[thread_id].signals;
    task_table[thread_id].signals &= ~(signal & 0x7fffffff);

    return prev_signals;
}

/**
 * Get signals of the currently running thread
 * @return Signals
 */
int32_t sched_threadSignalGetCurrent(void)
{
    return current_thread->signals;
}

/**
 * Get signals of a thread
 * @param thread_id Thread id.
 * @return          Signals of the selected thread.
 */
int32_t sched_threadSignalGet(osThreadId thread_id)
{
    return task_table[thread_id].signals;
}

/**
 * Wait for a signal(s)
 * @param signals   Signals that can woke-up the suspended thread.
 * @millisec        Timeout if selected signal is not invoken.
 * @return          Event that triggered resume back to running state.
 */
osEvent * sched_threadSignalWait(int32_t signals, uint32_t millisec)
{
    int tim;

    current_thread->event.status = osEventTimeout;

    if (millisec != (uint32_t)osWaitForever) {
        if ((tim = timers_add(current_thread->id, osTimerOnce, millisec)) < 0) {
            /* Timer error will be most likely but not necessarily returned
             * as is. There is however a slight chance of event triggering
             * before control returns back to this thread. It is completely OK
             * to clear this error if that happens. */
            current_thread->event.status = osErrorResource;
        }
        current_thread->wait_tim = tim;
    }

    if (current_thread->event.status != osErrorResource) {
        current_thread->sig_wait_mask = signals;
        _sched_thread_sleep_current();
    }

    /* Event status is now timeout but will be changed if any event occurs
     * as event is returned as a pointer. */
    return (osEvent *)(&(current_thread->event));
}

#if configDEVSUBSYS == 1
/**
 * Wait for device
 * @param dev Device that should be waited for; 0 = reset;
 */
osEvent * sched_threadDevWait(osDev_t dev, uint32_t millisec)
{
    current_thread->dev_wait = DEV_MAJOR(dev);

    if (dev == 0) {
        current_thread->event.status = osOK;
        return (osEvent *)(&(current_thread->event));
    }

    return sched_threadSignalWait(SCHED_DEV_WAIT_BIT, millisec);
}
#endif

/**
  * @}
  */

/**
  * @}
  */
