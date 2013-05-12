/**
 *******************************************************************************
 * @file    kernel.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 *
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#include "hal_core.h"
#include "syscall.h"
#include "kernel.h"

/* Kernel Control Functions **************************************************/

int32_t osKernelRunning(void)
{
      return 1;
}

/* Non-CMSIS *****************************************************************/

void osGetLoadAvg(uint32_t loads[3])
{
    syscall(KERNEL_SYSCALL_SCHED_GET_LOADAVG, loads);
}

/**
  * @}
  */
