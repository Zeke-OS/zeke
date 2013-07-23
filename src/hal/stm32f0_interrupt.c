/**
 *******************************************************************************
 * @file    stm32f0_interrupt.c
 * @author  Olli Vanhoja
 * @brief   Interrupt service routines.
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

#include "sched.h"
#include "syscall.h"
#include "hal_core.h"
#include <stm32f0xx.h>
#include <stm32f0xx_rcc.h>
#include "stm32f0_interrupt.h"

/**
 * Magic value that tells how many 32-bit words should be discarded from the
 * MSP before returning from context_switch.
 * @note This mainly depends on optimization parameters, compiler, code changes
 * in interrupt handler and current planetary positions during compile time.
 */
#define STM32F0_MAGIC_STACK_ADD_VALUE 2

static inline void run_scheduler(void);

int interrupt_init_module(void)
{
    RCC_ClocksTypeDef RCC_Clocks;

    /* Configure SysTick */
    RCC_GetClocksFreq(&RCC_Clocks);
    if (SysTick_Config(RCC_Clocks.HCLK_Frequency / configSCHED_FREQ)) {
        return -1; /* Error */
    }

    NVIC_SetPriority(PendSV_IRQn, 0x03); /* Set PendSV to lowest level */
    NVIC_SetPriority(SysTick_IRQn, 0x03); /* Configure the SysTick handler
                                           * priority */

    return 0; /* OK */
}

static inline void run_scheduler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
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
#pragma optimize = no_cse
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
