/**
 *******************************************************************************
 * @file    sched.h
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

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include "heap.h"
#include "timers.h"
#include "hal_core.h"
#include "sched.h"

/* When these flags are both set for a it's ok to make a context switch to it. */
#define SCHED_CSW_OK_FLAGS  (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG)

volatile uint32_t sched_enabled = 0; /* If this is set to != 0 interrupt
                                      * handlers will be able to call context
                                      * switching. */

threadInfo_t task_table[configSCHED_MAX_THREADS];
static heap_t priority_queue = HEAP_NEW_EMPTY;
volatile threadInfo_t * current_thread;

/* Varibles for CPU Load Calculation */
volatile uint32_t calcCPULoad = 1;
volatile uint32_t sched_cpu_load;

/* Stack for idle task */
static char sched_idle_stack[sizeof(sw_stack_frame_t) + sizeof(hw_stack_frame_t) + 100];
volatile int _first_switch = 1;


/* Private function prototypes -----------------------------------------------*/
void idleTask(void * arg);
void del_thread(void);
void context_switcher(void);
void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent);
void sched_thread_set_inheritance(osThreadId i, threadInfo_t * parent);
static void _sched_thread_set_exec(int thread_id, osPriority pri);
static void _sched_thread_sleep_current(void);

/**
  * Initialize the scheduler
  */
void sched_init(void)
{
    /* Create the idle task as task 0 */
    osThreadDef_t tdef_idle = { (os_pthread)(&idleTask), osPriorityIdle, sched_idle_stack, sizeof(sched_idle_stack)/sizeof(char) };
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

/** @todo CPU load calculation is now totally broke */
void idleTask(void * arg)
{
#ifndef PU_TEST_BUILD
    while(1) {
        if (calcCPULoad) {
            /* CPU Load Calculation */
            /* uint32_t countflag = SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk;
            uint32_t r = SysTick->LOAD & SysTick_LOAD_RELOAD_Msk;
            uint32_t v = (countflag) ? 0 : SysTick->VAL;
            sched_cpu_load = (r-v) / (r/100);

            calcCPULoad = 0;
            if (countflag) {
                req_context_switch();
            } */
        }
    }
#endif
}

/** @todo remove childs when parent is deleted */
void del_thread(void)
{
    current_thread->flags = 0; /* Clear all the flags */

    /* There should be no need to clear events or anything else as those are
     * not passed by pointer to anywhere and secondly they are cleared when
     * new thread is created.
     *
     * After flags is set to 0 the thread should be automatically removed
     * from the scheduler heap when it hits the top.
     */

    req_context_switch();
    while(1); /* Once the context changes, the program will no longer return to
               * this thread */
}

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
    context_switcher();
    load_context(); /* Since PSP has been updated, this loads the last state of
                     * the new task */
}

void context_switcher(void)
{
    int i;

    timers_run();

    /* Select next thread */
    do {
        /* TODO Remove re-populate code and move its functionality to the other functions!? */
        /* Need to re-populate the priority queue? */
        if ((priority_queue.size < 0)) {
            for (i = 0; i < configSCHED_MAX_THREADS; i++) {
                task_table[i].uCounter = 0;

                if ((task_table[i].flags & SCHED_CSW_OK_FLAGS) == SCHED_CSW_OK_FLAGS) {
                    (void)heap_insert(&priority_queue, &(task_table[i]));
                }
            }
        }

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
            if ((int)current_thread->priority >= 0) { /* Positive priority */
                tslice_n_max = configSCHED_MAX_SLICES * ((int)current_thread->priority + 1);
            } else { /* Negative priority */
                tslice_n_max = configSCHED_MAX_SLICES / (-1 * (int)current_thread->priority + 1);
            }

            if ((current_thread->flags & SCHED_EXEC_FLAG) == 0) {
                /* Remove the current thread from the priority queue */
                (void)heap_del_max(&priority_queue);
                /* Thread is in sleep so revert back to its original priority */
                current_thread->priority = current_thread->def_priority;
                current_thread->uCounter = 0;
            } else if ((current_thread->uCounter >= (uint32_t)tslice_n_max) /* if maximum slices for this thread */
              && ((int)current_thread->priority > (int)osPriorityBelowNormal)) /* if priority is higher than this */
            {
                /* Give a penalty: Set lower priority and
                 * perform heap decrement operation. */
                current_thread->priority = osPriorityBelowNormal;
                heap_dec_key(&priority_queue, 0);
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
void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent)
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

void sched_thread_set_inheritance(osThreadId i, threadInfo_t * parent)
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

/**
 * @todo Plz
 */
static void _sched_thread_sleep_current(void)
{
    int i = 0;

    /* Sleep flag */
    current_thread->flags &= ~SCHED_EXEC_FLAG;

    /* Following should remove the thread from heap immediately when context
     * switch is called. */
    current_thread->priority = osPriorityError;

    /* Why oh why :'( */
    for (i = 0; i < priority_queue.size; i++) {
        if (priority_queue.a[i]->id == current_thread->id)
            break;
    }

    heap_inc_key(&priority_queue, i);
    heap_del_max(&priority_queue);
}


/* Functions defined in header file (and used mainly by syscalls) */

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

/** @todo Doesn't support other than osWaitForever atm */
osStatus sched_threadDelay(uint32_t millisec)
{
    if (millisec == osWaitForever) {
        _sched_thread_sleep_current();              /* Sleep */
        current_thread->flags |= SCHED_NO_SIG_FLAG; /* Shouldn't get woken up by signals */
        current_thread->event.status = osOK;
    } else {
        if (timers_add(current_thread->id, millisec) == 0) {
            _sched_thread_sleep_current();
            current_thread->flags |= SCHED_NO_SIG_FLAG; /* Shouldn't get woken up by signals */
            current_thread->event.status = osOK;
        } else {
             current_thread->event.status = osErrorResource;
        }
    }

    return current_thread->event.status;
}

/** @todo Doesn't support other than osWaitForever atm */
uint32_t sched_threadWait(uint32_t millisec)
{
    if (millisec == osWaitForever) {
        current_thread->flags |= SCHED_NO_SIG_FLAG; /* Shouldn't get woken up by signals */
    }
    _sched_thread_sleep_current();                  /* Sleep */

    current_thread->event.status = osOK;
    return (uint32_t)(&current_thread->event);
}

uint32_t sched_threadSetSignal(osThreadId thread_id, int32_t signal)
{
    uint32_t prev_signals;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000;

    prev_signals = (uint32_t)task_table[thread_id].event.value.signals;

    task_table[thread_id].event.value.signals = signal;
    task_table[thread_id].event.status = osEventSignal;

    if ((task_table[thread_id].flags & SCHED_NO_SIG_FLAG) == SCHED_NO_SIG_FLAG) {
        /* Set the signaled thread back into execution */
        _sched_thread_set_exec(thread_id, task_table[thread_id].def_priority);
    }

    return prev_signals;
}
