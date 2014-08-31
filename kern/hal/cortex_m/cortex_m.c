/**
 *******************************************************************************
 * @file    cortex_m.c
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for Cortex-M
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup HAL
  * @{
  */

/** @addtogroup Cortex-M
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#if configARCH == __ARM7M__ || configARCH == __ARM7EM__
/* Needed for debugging if __ARM7M__ or __ARM7EM__ */
#include <kstring.h>
#endif
#include <stddef.h>
#include <tsched.h>
#include "../hal_mcu.h"
#include "cortex_m.h"

uint32_t flag_kernel_tick = 0;

/* Core specific hard fault handlers */
#if configARCH == __ARM6M__ || configARCH == __ARM6SM__
void hard_fault_handler_armv6m(uint32_t stack[]);
#elif configARCH == __ARM7M__ || configARCH == __ARM7EM__
void hard_fault_handler_armv7m(uint32_t stack[]);
void stackDump(uint32_t stack[]);
static void printErrorMsg(const char * errMsg);
#endif

void init_stack_frame(osThreadDef_t * thread_def, void * argument, uint32_t a_del_thread)
{
    hw_stack_frame_t * thread_frame;

    /* Pointer to the thread hw stack frame */
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

int test_and_set(int * lock) {
    int old_value;

    /* Ensure that all explicit memory accesses that appear in program order
     * before the DMB instruction are observed before any explicit memory
     * accesses. */
    __asm__ volatile ("DMB");
    old_value = *lock;
    *lock = 1;

    return old_value;
}

/* HardFault Handling *********************************************************/

void HardFault_Handler(void)
{
    /* First call the core specific HardFault handler */
#if configARCH == __ARM6M__ || configARCH == __ARM6SM__
    int scratch;
    __asm__ volatile ("PUSH {LR}\n\t"
                      "POP {%0}\n"
                      : "=r" (scratch));
    if (scratch == HAND_RETURN) {
        __asm__ volatile ("MRS R0, MSP\n\t"
                          "B hard_fault_handler_armv6m\n");
    } else {
        __asm__ volatile ("MRS R0, PSP\n\t"
                          "B hard_fault_handler_armv6m\n");
    }
#elif  configARCH == __ARM7M__ || configARCH == __ARM7EM__
    /* Using IT */
    __asm__ volatile ("TST LR, #4\n\t"
                      "ITE EQ\n\t"
                      "MRSEQ R0, MSP\n\t"
                      "MRSNE R0, PSP\n\t"
                      "B hard_fault_handler_armv7m\n");

#else
#error Support for this instruction set is not yet implemented.
#endif

    /* If core specific HardFault handler returns it means that we can safely
     * kill the current thread and call the scheduler for reschedule. */

    /* Kill the current thread */
    thread_terminate(current_thread->id);

    /* Return to the scheduler ASAP */
    req_context_switch();
}

/**
 * This function handles the Hard Fault exception on ARMv6M.
 * @param stack top of the stack.
 */
void hard_fault_handler_armv6m(uint32_t stack[])
{
    uint32_t thread_stack;

    __asm__ volatile ("MRS %0, PSP"
                      : "=r" (thread_stack));
    if ((uint32_t)stack != thread_stack) {
        /* Kernel fault */
        __asm__ volatile ("BKPT #01");
        while(1);
    }

    /* TODO It's possible to implement a stack dump code here if desired so. */
}

#if configARCH == __ARM7M__ || configARCH == __ARM7EM__
/* There is no HFSR register or ITM at least on Cortex-M0 and M1 (ARMv6) */

/**
 * This function handles the Hard Fault exception on ARMv7M.
 * @param stack top of the stack.
 */
void hard_fault_handler_armv7m(uint32_t stack[])
{
    static char msg[80];

    /* TODO just kill the process if thread sp is in use (and fault is not fatal)
     * and then continue operation */

    printErrorMsg("In Hard Fault Handler\n");
    sprintf(msg, "SCB->HFSR = 0x%08x", SCB->HFSR);
    printErrorMsg(msg);

    if ((SCB->HFSR & (1 << 30)) != 0) {
        printErrorMsg("Forced Hard Fault\n");
        sprintf(msg, "SCB->CFSR = 0x%08x", SCB->CFSR);
        printErrorMsg(msg);
        if((SCB->CFSR & 0xFFFF0000) != 0) {
            stackDump(stack);
        }
    }
    stackDump(stack);

    __asm__ volatile ("BKPT #01");
    while(1);
}
#endif

#if configARCH == __ARM7M__ || configARCH == __ARM7EM__
/* There is no ITM for Cortex-M0..1 */

/**
 * Print stack dump for the debugger.
 * @param stack top of the stack.
 */
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

/**
 * Print user defined error message to the debugger.
 * @param errMsg error message.
 */
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
