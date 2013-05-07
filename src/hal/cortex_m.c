/**
 *******************************************************************************
 * @file    cortex_m.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for Cortex-M
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup Cortex-M
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#if __CORE__ == __ARM7M__ || __CORE__ == __ARM7EM__
#include <stdio.h>
#include <string.h>
#endif
#include <stddef.h>
#include "sched.h"
#include "hal_mcu.h"
#include "cortex_m.h"

uint32_t flag_kernel_tick = 0;

#if __CORE__ == __ARM6M__ || __CORE__ == __ARM6SM__
void hard_fault_handler_armv6m(uint32_t stack[]);
#elif __CORE__ == __ARM7M__ || __CORE__ == __ARM7EM__
void hard_fault_handler_armv7m(uint32_t stack[]);
void stackDump(uint32_t stack[]);
static void printErrorMsg(const char * errMsg);
#endif

void init_hw_stack_frame(osThreadDef_t * thread_def, void * argument, uint32_t a_del_thread)
{
    hw_stack_frame_t * thread_frame;

    thread_frame = (hw_stack_frame_t *)((uint32_t)(thread_def->stackAddr)
                    + thread_def->stackSize - sizeof(hw_stack_frame_t));
    thread_frame->r0 = (uint32_t)(argument);
    thread_frame->r1 = 0;
    thread_frame->r2 = 0;
    thread_frame->r3 = 0;
    thread_frame->r12 = 0;
    thread_frame->pc = ((uint32_t)(thread_def->pthread));
    thread_frame->lr = a_del_thread;
    thread_frame->psr = DEFAULT_PSR;
}

/**
  * Make a system call
  */
#pragma optimize=no_code_motion
uint32_t syscall(int type, void * p)
{
    asm volatile(
                 "MOV r2, %0\n" /* Put parameters to r2 & r3 */
                 "MOV r3, %1\n"
                 "MOV r1, r4\n" /* Preserve r4 by using hardware push */
                 "SVC #0\n"
                 "DSB\n" /* Ensure write is completed
                          * (architecturally required, but not strictly
                          * required for existing Cortex-M processors) */
                 "ISB\n" /* Ensure PendSV is executed */
                 : : "r" (type), "r" (p));

    /* Get return value */
    osStatus scratch;
    asm volatile("MOV %0, r4\n"
                 "MOV r4, r1\n" /* Read r4 back from r1 */
                 : "=r" (scratch));

    return scratch;
}

int test_and_set(int * lock) {
    int old_value;

    /* Ensure that all explicit memory accesses that appear in program order
     * before the DMB instruction are observed before any explicit memory
     * accesses. */
    asm volatile("DMB");
    old_value = *lock;
    *lock = 1;

    return old_value == 1;
}

/* Fault Handling *************************************************************/

void HardFault_Handler(void)
{
#if __CORE__ == __ARM6M__ || __CORE__ == __ARM6SM__
    int scratch;
    asm volatile ("PUSH {LR}\n"
                  "POP {%0}\n"
                  : "=r" (scratch));
    if (scratch == HAND_RETURN) {
        asm volatile ("MRS R0, MSP\n"
        "B hard_fault_handler_armv6m\n");
    } else {
        asm volatile ("MRS R0, PSP\n"
        "B hard_fault_handler_armv6m\n");
    }
#elif  __CORE__ == __ARM7M__ || __CORE__ == __ARM7EM__
        /* Using IT */
    asm volatile ("TST LR, #4\n"
                  "ITE EQ\n"
                  "MRSEQ R0, MSP\n"
                  "MRSNE R0, PSP\n"
                  "B hard_fault_handler_armv7m\n");

#else
#error Support for this instruction set is not yet implemented.
#endif
}

/**
  * This function handles the Hard Fault exception on ARMv6M.
  */
void hard_fault_handler_armv6m(uint32_t stack[])
{
    uint32_t thread_stack;

    asm volatile("MRS %0, PSP\n"
                 : "=r" (thread_stack));
    if ((uint32_t)stack != thread_stack) {
        /* Kernel fault */
        asm volatile("BKPT #01");
        while(1);
    }

    /* TODO it's possible to implement some kind of stack dump here.
     *      If stack dump will not be implemented for ATMv6 then it would be
     *      wise to "optimize" this code into the actual handler. */

    /* Kill the current thread */
    sched_thread_terminate(current_thread->id);

    /* Return to the scheduler ASAP */
    req_context_switch();
    asm volatile ("BX %0\n"
              :
              : "r" (THREAD_RETURN)
              );
}

#if __CORE__ == __ARM7M__ || __CORE__ == __ARM7EM__
/* There is no HFSR register or ITM at least on Cortex-M0 and M1 (ARMv6) */

/**
  * This function handles the Hard Fault exception on ARMv7M.
  */
void hard_fault_handler_armv7m(uint32_t stack[])
{
    static char msg[80];

    /* TODO just kill the process if thread sp is in use (and fault is not fatal)
     * and then continue operation */

    printErrorMsg("In Hard Fault Handler\n");
    sprintf(msg, "SCB->HFSR = 0x%08x\n", SCB->HFSR);
    printErrorMsg(msg);

    if ((SCB->HFSR & (1 << 30)) != 0) {
        printErrorMsg("Forced Hard Fault\n");
        sprintf(msg, "SCB->CFSR = 0x%08x\n", SCB->CFSR);
        printErrorMsg(msg);
        if((SCB->CFSR & 0xFFFF0000) != 0) {
            stackDump(stack);
        }
    }
    stackDump(stack);

    asm volatile("BKPT #01");
    while(1);
}
#endif

#if __CORE__ == __ARM7M__ || __CORE__ == __ARM7EM__
/* There is no ITM for Cortex-M0..1 */

static void stackDump(uint32_t stack[])
{
    static char msg[80];

    sprintf(msg, "r0  = 0x%08x\n", stack[0]);  printErrorMsg(msg);
    sprintf(msg, "r1  = 0x%08x\n", stack[1]);  printErrorMsg(msg);
    sprintf(msg, "r2  = 0x%08x\n", stack[2]);  printErrorMsg(msg);
    sprintf(msg, "r3  = 0x%08x\n", stack[3]);  printErrorMsg(msg);
    sprintf(msg, "r12 = 0x%08x\n", stack[4]); printErrorMsg(msg);
    sprintf(msg, "lr  = 0x%08x\n", stack[5]);  printErrorMsg(msg);
    sprintf(msg, "pc  = 0x%08x\n", stack[6]);  printErrorMsg(msg);
    sprintf(msg, "psr = 0x%08x\n", stack[7]); printErrorMsg(msg);
}

static void printErrorMsg(const char * errMsg)
{
    while (*errMsg != '\0') {
        ITM_SendChar(*errMsg);
        ++errMsg;
    }
}
#endif

/* End of Fault Handling ******************************************************/

/**
  * @}
  */

/**
  * @}
  */
