/**
 *******************************************************************************
 * @file    wait.c
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

/** @addtogroup ThreadScope
  * @{
  */

/** @addtogroup Wait
  * @{
  */

/* Generic Wait Functions ****************************************************/

osStatus osDelay(uint32_t millisec)
{
    osStatus result;

    result = (osStatus)syscall(KERNEL_SYSCALL_SCHED_DELAY, &millisec);

    if (result != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    return result;
}

osEvent osWait(uint32_t millisec)
{
    osEvent * result;

    result = (osEvent *)syscall(KERNEL_SYSCALL_SCHED_WAIT, &millisec);

    if (result->status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    return *result;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
