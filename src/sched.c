/**
 *******************************************************************************
 * @file    sched.h
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler header
 *******************************************************************************
 */

/** @addtogroup Scheduler
  * @{
  */

#include <stdint.h>
#include <intrinsics.h>
#include <stddef.h>
#include <string.h>
#include "stm32f0xx.h"

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include "heap.h"
#include "sched.h"
#include "kernel_config.h"

#ifndef __ARM_PROFILE_M__
    #error Only ARM Cortex-M profile is currently supported.
#endif
#ifndef __CORE__
    #error Core is not selected by the compiler.
#endif

#define MAIN_RETURN     0xFFFFFFF9 /* Return using the MSP */
#define THREAD_RETURN   0xFFFFFFFD /* Return using the PSP */

/* stack frame saved by the hardware */
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} hw_stack_frame_t;

/* Stack frame save by the software */
typedef struct {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
} sw_stack_frame_t;

static uint32_t * stack;    /* Position in MSP stack where special return codes
                             * can be written. This is mainly in a memory space
                             * of some interrupt handler. */
volatile uint32_t sched_enabled = 0; /* If this is set to != 0 interrupt
                                      * handlers will be able to call context
                                      * switching. */

threadInfo_t task_table[configSCHED_MAX_THREADS];
static heap_t priority_queue = {{NULL}, -1};
volatile threadInfo_t * current_thread;

/* Varibles for CPU Load Calculation */
volatile uint32_t calcCPULoad = 1;
volatile uint32_t sched_cpu_load;

static char m_stack[sizeof(sw_stack_frame_t)]; /* Stack for main */
static char sched_i_stack[sizeof(sw_stack_frame_t) + sizeof(hw_stack_frame_t) + 100]; /* Stack for idle task */


/* Private function prototypes -----------------------------------------------*/
void idleTask(void * arg);
void del_thread(void);
static inline void * rd_stack_ptr(void);
static inline void save_context(void);
static inline void load_context(void);
static inline void * rd_thread_stack_ptr(void);
static inline void wr_thread_stack_ptr(void * ptr);
void context_switcher(void);

/**
  * Initialize the scheduler
  */
void sched_init(void)
{
    task_table[0].sp = m_stack + sizeof(sw_stack_frame_t);
    current_thread = &(task_table[0]);

    /* Create idle task */
    osThreadDef_t idle = { (os_pthread)(&idleTask), osPriorityIdle, sched_i_stack, sizeof(sched_i_stack)/sizeof(char), NULL };
    sched_ThreadCreate(&idle);
}

/**
  * Start scheduler
  */
void sched_start(void)
{
    __istate_t s = __get_interrupt_state();
    __disable_interrupt();

    sched_enabled = 1;

    __set_interrupt_state(s); /* Restore interrupts */

    /* Scheduler should now start */
}

void idleTask(void * arg)
{
    while(1) {
        if (calcCPULoad) {
            /* CPU Load Calculation */
            uint32_t countflag = SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk;
            uint32_t r = SysTick->LOAD & SysTick_LOAD_RELOAD_Msk;
            uint32_t v = (countflag) ? 0 : SysTick->VAL;
            sched_cpu_load = (r-v) / (r/100);

            calcCPULoad = 0;
            if (countflag) {
                SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* Set PendSV pending status */
                asm volatile("DSB\n" /* Ensure write is completed
                                      * (architecturally required, but not strictly
                                      * required for existing Cortex-M processors) */
                             "ISB\n" /* Ensure PendSV is executed */
                );
            }
        }
    }
}

void del_thread(void)
{
    current_thread->flags = 0; /* Clear all the flags */
    SCB->ICSR |= (1<<28); /* Switch the context */
    while(1); /* Once the context changes, the program will no longer return to
               * this thread */
}

/**
  * Save the context on the PSP
  */
static inline void save_context(void)
{
    volatile uint32_t scratch;
#if __CORE__ == __ARM6M__
    asm volatile ("MRS   %0,  psp\n"
                  "SUBS  %0,  %0, #32\n"
                  "MSR   psp, %0\n"         /* This is the address that will use by rd_thread_stack_ptr(void) */
                  "ISB\n"
                  "STMIA %0!, {r4-r7}\n"
                  "PUSH  {r4-r7}\n"         /* Push original register values so we don't lost them */
                  "MOV   r4,  r8\n"
                  "MOV   r5,  r9\n"
                  "MOV   r6,  r10\n"
                  "MOV   r7,  r11\n"
                  "STMIA %0!, {r4-r7}\n"
                  "POP   {r4-r7}\n"         /* Pop them back */
                  : "=r" (scratch));
#elif __CORE__ == __ARM7M__
    asm volatile ("MRS   %0,  psp\n"
                  "STMDB %0!, {r4-r11}\n"
                  "MSR   psp, %0\n"
                  "ISB\n"
                  : "=r" (scratch));
#else
    #error Selected CORE not supported
#endif
}

/**
  * Load the context from the PSP
  */
static inline void load_context(void)
{
    volatile uint32_t scratch;
#if __CORE__ == __ARM6M__
    asm volatile ("MRS   %0,  psp\n"
                  "ADDS  %0,  %0, #16\n"      /* Move to the high registers */
                  "LDMIA %0!, {r4-r7}\n"
                  "MOV   r8,  r4\n"
                  "MOV   r9,  r5\n"
                  "MOV   r10, r6\n"
                  "MOV   r11, r7\n"
                  "MSR   psp, %0\n"           /* Store the new top of the stack */
                  "ISB\n"
                  "SUBS  r0,  r0, #32\n"      /* Go back to the low registers */
                  "LDMIA %0!, {r4-r7}\n"
                  : "=r" (scratch));
#elif __CORE__ == __ARM7M__
    asm volatile ("MRS   %0,  psp\n"
                  "LDMFD %0!, {r4-r11}\n"
                  "MSR   psp, %0\n"
                  "ISB\n"
                  : "=r" (scratch));
#else
    #error Selected CORE not supported
#endif
}

/**
  * Read the main stack pointer
  */
static inline void * rd_stack_ptr(void)
{
    void * result = NULL;
    asm volatile("MRS %0, msp\n"
                 : "=r" (result));
    return result;
}

/**
  * Read the PSP so that it can be stored in the task table
  */
static inline void * rd_thread_stack_ptr(void)
{
    void * result = NULL;
    asm volatile ("MRS %0, psp\n" : "=r" (result));
    return(result);
}

/**
  * Write stack pointer of the currentthread to the PSP
  */
static inline void wr_thread_stack_ptr(void * ptr)
{
    asm volatile ("MSR psp, %0\n"
                  "ISB\n" : : "r" (ptr));
}

/**
  * sysTick interrupt handler
  *
  * This grabs the main stack value and then calls the context swither.
  */
void sched_handler(void * st)
{
    stack = (uint32_t *)st;
    if (current_thread == &(task_table[0])) {
        /* Copy MSP to PSP */
        volatile uint32_t scratch;
        asm volatile ("MRS %0,  msp\n"
                      "MSR psp, %0\n"
                      "ISB\n"
                      : "=r" (scratch));
    } else {
        save_context();
    }
    context_switcher();
    load_context(); /* Since PSP has been updated, this loads the last state of
                     * the new task */
}

void context_switcher(void)
{
    /* Save the current task's stack pointer */
    current_thread->sp = rd_thread_stack_ptr();

    /* Select next thread */
    do {
        /* Need to repopulate the priority queue? */
        if ((priority_queue.size < 0)) {
            calcCPULoad = 1; /* We should calculate the CPU load */
            for (int i = 1; i < configSCHED_MAX_THREADS; i++) {
                task_table[i].uCounter = 0;

                if (task_table[i].flags & SCHED_EXEC_FLAG) {
                    (void)heap_insert(&priority_queue, &(task_table[i]));
                }
            }
        } else {
            /* Check if current thread can be removed from the queue */
            if (current_thread == priority_queue.a[0]) {
                if (((current_thread->flags & SCHED_EXEC_FLAG) == 0)
                    || (current_thread->uCounter > configSCHED_MAX_CYCLES)
                    || (current_thread->priority == osPriorityIdle)) {
                    (void)heap_del_max(&priority_queue);
                }
            }
        }

        /* Next thread is the top one on the priority queue */
        current_thread = *priority_queue.a;

        /* In the while loop condition we'll skip this thread if its EXEC flag
        * is disabled. */
    } while ((current_thread->flags & SCHED_EXEC_FLAG) == 0);

    /* The counter will help us to see how often the task is getting its turn to run */
    current_thread->uCounter++;

    /* Use the thread stack upon handler return */
    *((uint32_t *)stack) = THREAD_RETURN;

    /* Write the value of the PSP for the next thread in run state */
    wr_thread_stack_ptr(current_thread->sp);
}

/**
  * Create a new thread
  *
  */
osThreadId sched_ThreadCreate(osThreadDef_t * thread_def)
{
    int i;
    hw_stack_frame_t * thread_frame;

    /* Disable context switching to support multi-threaded calls to this fn */
    __istate_t s = __get_interrupt_state();
    __disable_interrupt();

    for (i = 1; i < configSCHED_MAX_THREADS; i++) {
        if (task_table[i].flags == 0) {
            thread_frame = (hw_stack_frame_t *)((uint32_t)(thread_def->stackAddr) + thread_def->stackSize - sizeof(hw_stack_frame_t));
            thread_frame->r0 = (uint32_t)(thread_def->argument);
            thread_frame->r1 = 0;
            thread_frame->r2 = 0;
            thread_frame->r3 = 0;
            thread_frame->r12 = 0;
            thread_frame->pc = ((uint32_t)(thread_def->pthread));
            thread_frame->lr = (uint32_t)del_thread;
            thread_frame->psr = 0x21000000; /* Default PSR value */

            task_table[i].flags = SCHED_IN_USE_FLAG | SCHED_EXEC_FLAG;
            task_table[i].priority = thread_def->tpriority;
            memset(&(task_table[i].event), 0, sizeof(osEvent));
            task_table[i].uCounter = 0;
            task_table[i].sp = (void *)((uint32_t)(thread_def->stackAddr) +
                               thread_def->stackSize -
                               sizeof(hw_stack_frame_t) -
                               sizeof(sw_stack_frame_t));

            break;
        }
    }
    __set_interrupt_state(s); /* Restore interrupts */

    if (i == configSCHED_MAX_THREADS) {
        /* New thread could no be created */
        return NULL;
    } else {
        /* Return the id of the new thread */
        return (osThreadId)i;
    }
}

/** @todo Doesn't support other than osWaitForever atm */
osStatus sched_threadDelay(uint32_t millisec)
{
    if (millisec != osWaitForever) {
        return osErrorParameter; /* Not supported yet */
    }
    current_thread->flags &= ~SCHED_EXEC_FLAG;      /* Sleep */
    current_thread->flags &= ~SCHED_NO_SIG_FLAG;    /* Shouldn't get woken up by signals */
    current_thread->event.status = osOK;

    return osOK;
}

/** @todo Doesn't support other than osWaitForever atm */
uint32_t sched_threadWait(uint32_t millisec)
{
    if (millisec != osWaitForever) {
        current_thread->event.status = osErrorParameter;
        return (uint32_t)(&current_thread->event);  /* Not supported yet */
    }
    current_thread->flags &= ~SCHED_EXEC_FLAG;      /* Sleep */

    current_thread->event.status = osOK;
    return (uint32_t)(&current_thread->event);
}

uint32_t sched_threadSetSignal(osThreadId thread_id, int32_t signal)
{
    uint32_t prev_signals = (uint32_t)task_table[thread_id].event.value.signals;

    task_table[thread_id].event.value.signals = signal;
    task_table[thread_id].event.status = osEventSignal;

    if ((task_table[thread_id].flags & SCHED_NO_SIG_FLAG) == 0) {
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
    }

    return prev_signals;
}
