/**
 *******************************************************************************
 * @file    stm32f0_interrupt.c
 * @author  Olli Vanhoja
 * @brief Interrupt service routines.
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

/** @addtogroup STM32F0
  * @{
  */

#include "stm32f0_interrupt.h"
#include "sched.h"
#include "kernel_config.h"

#define SYSTICK_DIVIDER         SystemCoreClock / configSCHED_FREQ


int interrupt_init_module(void)
{
    NVIC_SetPriority(PendSV_IRQn, 0x03); /* Set PendSV to lowest level */

    /* Setup SysTick Timer for tLevel ms interrupts  */
    if (SysTick_Config(SystemCoreClock / SYSTICK_DIVIDER))
    {
        /* Capture error */
        while (1);
    }

    /* Configure the SysTick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x03);

    return 0;
}

/**
  * This function handles NMI exception.
  */
void NMI_Handler(void)
{
    /* @todo fixme */
}

/**
  * This function handles Hard Fault exception.
  */
void HardFault_Handler(void)
{
    /* @todo fixme */
    while (1)
    { }
}

/**
  * This function handles SVCall exception.
  */
void SVC_Handler(void)
{
    /* @todo fixme */
}

/**
  * This function handles PendSVC exception.
  */
void PendSV_Handler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        void * result = NULL;
        asm volatile("MRS %0, msp\n"
                     : "=r" (result));

        sched_handler(result);
        asm volatile ("POP {r0}\n"
                      "ADD sp, sp, #(4)\n"
                      "BX  r0");
    }
}

/**
  * This function handles SysTick Handler.
  */
void SysTick_Handler(void)
{
    /* Run scheduler */
    if (sched_enabled) {
        void * result = NULL;
        asm volatile("MRS %0, msp\n"
                     : "=r" (result));

        sched_handler(result);
        asm volatile ("POP {r0}\n"
                      "ADD sp, sp, #(4)\n"
                      "BX  r0");
    }
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
