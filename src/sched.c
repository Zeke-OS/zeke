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

#include "heap.h"
#include "kernel.h"
#include "sched.h"
#include "kernel_config.h"

#ifndef __ARM_PROFILE_M__
    #error Only ARM Cortex-M profile is currently supported
#endif

#define MAIN_RETURN     0xFFFFFFF9 /* Return using the MSP */
#define THREAD_RETURN   0xFFFFFFFD /* Return using the PSP */

typedef struct {
    void * sp; /*!< Stack pointer */
    int flags; /*!< Status flags */
    osPriority priority; /*!< Task priority */
} task_table_t;

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

task_table_t task_table[configSCHED_MAX_THREADS];
static int heap_arr[configSCHED_MAX_THREADS];
static heap_t priority_queue = {heap_arr, 0, configSCHED_MAX_THREADS};
volatile int current_thread_ind;  /* Currently running thread */
volatile int next_thread_ind;

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
    current_thread_ind = 0;
    next_thread_ind = 1;

    /* Create idle task */
    kernel_ThreadCreate(&idleTask, NULL, sched_i_stack, sizeof(sched_i_stack)/sizeof(char));
}

/**
  * Start scheduler
  * @todo idle task should be added as a last task
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
            if (countflag)
                kernel_ThreadSleep();
        }
    }
}

/**
  * Create a new thread
  *
  * @param p Pointer to the new task or task initializer function
  * @param Argument
  * @param stackAddr Stack allocation reserved by the task
  * @param stackSize Size of stack reserved for the task
  */
int kernel_ThreadCreate(void (*p)(void*), void const * arg, void * stackAddr, size_t stackSize)
{
    int i;
    hw_stack_frame_t * thread_frame;

    /* Disable context switching to support multi-threaded calls to this fn */
    __disable_interrupt();
    for (i = 1; i < configSCHED_MAX_THREADS; i++) {
        if (task_table[i].flags == 0) {
            thread_frame = (hw_stack_frame_t *)((uint32_t)stackAddr + stackSize - sizeof(hw_stack_frame_t));
            thread_frame->r0 = (uint32_t)arg;
            thread_frame->r1 = 0;
            thread_frame->r2 = 0;
            thread_frame->r3 = 0;
            thread_frame->r12 = 0;
            thread_frame->pc = ((uint32_t)p);
            thread_frame->lr = (uint32_t)del_thread;
            thread_frame->psr = 0x21000000; /* Default PSR value */
            task_table[i].flags = SCHED_IN_USE_FLAG | SCHED_EXEC_FLAG;

            task_table[i].sp = (void *)((uint32_t)stackAddr +
                               stackSize -
                               sizeof(hw_stack_frame_t) -
                               sizeof(sw_stack_frame_t));

            break;
        }
    }
    __enable_interrupt(); /* Enable context switching */
    if (i == configSCHED_MAX_THREADS) {
        /* New thread could no be created */
        return 0;
    } else {
        /* Return the id of the new thread */
        return i;
    }
}

void del_thread(void)
{
    task_table[current_thread_ind].flags = 0; /* Clear all the flags */
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
                  "MSR   psp, %0\n"        /* This is the address that will use by rd_thread_stack_ptr(void) */
                  "ISB\n"
                  "STMIA %0!, {r4-r7}\n"
                  "MOV   r4,  r8\n"
                  "MOV   r5,  r9\n"
                  "MOV   r6,  r10\n"
                  "MOV   r7,  r11\n"
                  "STMIA %0!, {r4-r7}\n"
                  : "=r" (scratch));
#elif __CORE__ == __ARM7M__
    asm volatile ("MRS   %0,  psp\n"
                  "STMDB %0!, {r4-r11}\n"
                  "MSR   psp, %0\n"
                  "ISB\n"
                  : "=r" (scratch));
#else
    #error Selected CORE is not supported
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
    #error Selected CORE is not supported
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
    if (current_thread_ind == 0) {
        /* Copy MSP to PSP */
        volatile uint32_t scratch;
        asm volatile ("MRS %0,  msp\n"
                      "MSR psp, %0\n"
                      "ISB\n"
                      : "=r" (scratch));
    }
    stack = (uint32_t *)st;
    save_context();
    context_switcher();
    load_context(); /* Since PSP has been updated, this loads the last state of
                     * the new task */
}

/* @todo fixme */
void context_switcher(void)
{
    /* Save the current task's stack pointer */
    task_table[current_thread_ind].sp = rd_thread_stack_ptr();

    /* This is awful implementation of priority notifiers, please fix me! */
    do {
        if (next_thread_ind >= configSCHED_MAX_THREADS) {
            uint32_t tmp = SysTick->CTRL; // Clear COUNTFLAG
            calcCPULoad = 1;
            next_thread_ind = 1;
        }

        /* @todo Wake-up threads if signaled etc. */


        current_thread_ind = next_thread_ind;
    } while ((task_table[next_thread_ind++].flags & SCHED_EXEC_FLAG) == 0);

    /* Use the thread stack upon handler return */
    *((uint32_t *)stack) = THREAD_RETURN;

    /* Write the value of the PSP for the next thread in run state */
    wr_thread_stack_ptr(task_table[current_thread_ind].sp);
}

/** @todo temporary implementation of sleep */
void kernel_ThreadSleep_ms(uint32_t delay)
{
    do {
        delay--;
        kernel_ThreadSleep();
    } while (delay != 0);

}

/**
  * Put thread in wait state
  */
void kernel_ThreadWait(void)
{
    __istate_t s = __get_interrupt_state();
    __disable_interrupt();

    task_table[current_thread_ind].flags &= ~SCHED_EXEC_FLAG; /* Disable exec flag */

    __set_interrupt_state(s); /* Restore interrupts */

    kernel_ThreadSleep(); /* Put thread in sleep */
}

/** @todo fixme */
void kernel_ThreadNotify(int thread_id)
{
}
