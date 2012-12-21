/**
 *******************************************************************************
 * @file    kernel.h
 * @author  Olli Vanhoja
 * @brief   Zero Kernel
 *
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#include "stm32f0_interrupt.h"

#include "sched.h"
#include "kernel.h"

void kernel_init(void)
{
    interrupt_init_module();
}

void kernel_start(void)
{
    sched_init();
    sched_start();
    while(1) { }
}

/**
  * @}
  */