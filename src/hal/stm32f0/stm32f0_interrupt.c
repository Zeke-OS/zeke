/**
 *******************************************************************************
 * @file    stm32f0_interrupt.c
 * @author  Olli Vanhoja
 * @brief   Interrupt service routines.
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

/** @addtogroup STM32F0
  * @{
  */

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <sched.h>
#include <syscall.h>
#include <stm32f0xx_conf.h>
#include <stm32f0xx.h>
#include <stm32f0xx_rcc.h>
#include "../hal_core.h"
#include "stm32f0_interrupt.h"

/**
 * Magic value that tells how many 32-bit words should be discarded from the
 * MSP before returning from context_switch.
 * @note This mainly depends on optimization parameters, compiler, code changes
 * in interrupt handler and current planetary positions during compile time.
 */
#define STM32F0_MAGIC_STACK_ADD_VALUE 2

void interrupt_init_module(void) __attribute__((constructor));
static inline void run_scheduler(void);

void interrupt_init_module(void)
{
    RCC_ClocksTypeDef RCC_Clocks;

    /* Configure SysTick */
    RCC_GetClocksFreq(&RCC_Clocks);
    if (SysTick_Config(RCC_Clocks.HCLK_Frequency / configSCHED_FREQ)) {
        while (1); /* Error */
    }

    NVIC_SetPriority(PendSV_IRQn, 0x03); /* Set PendSV to lowest level */
    NVIC_SetPriority(SysTick_IRQn, 0x03); /* Configure the SysTick handler
                                           * priority */
}

static inline void run_scheduler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        /* Non-hw backed registers should remain untouched before this point */
        save_context();
        sched_handler();
        load_context(); /* Since PSP has been updated, this loads the last state
                         * of the resumed task */
        __asm__ volatile ("ADD sp, sp, %0\n"
                          "BX %1\n"
                          :
                          : "i" (STM32F0_MAGIC_STACK_ADD_VALUE * 4),
                          "r" (THREAD_RETURN)
                         );
    }
}

/**
  * This function handles NMI exception.
  */
void NMI_Handler(void)
{
    /* @todo fixme */
}

/**
  * This function handles SVCall exception.
  */
void SVC_Handler(void)
{
    uint32_t type;
    void * p;

    __asm__ volatile("MOV %0, r2\n" /* Read parameters from r2 & r3 */
                     "MOV %1, r3\n"
                     : "=r" (type), "=r" (p));

    /* Call kernel's internal syscall handler */
    uint32_t result = _intSyscall_handler(type, p);

    /* Return value is stored to r4 */
    __asm__ volatile("MOV r4, %0\n"
                     : : "r" (result));
}

/**
  * This function handles PendSVC exception.
  */
void PendSV_Handler(void)
{
	run_scheduler();
}

/**
  * This function handles SysTick Handler.
  */
void SysTick_Handler(void)
{
	run_scheduler();
}

/**
  * This function handles USART interrupt request.
  */
void USART1_IRQHandler(void)
{
    /* @todo fixme */
}

/**
  * @}
  */

/**
  * @}
  */
