/**
 *******************************************************************************
 * @file    devsys.c
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

/** @addtogroup Dev_subsys
  * @{
  */

/* Non-CMSIS *****************************************************************/

#if configDEVSUBSYS == 1
osEvent osDevWait(osDev_t dev, uint32_t millisec)
{
    ds_osDevWait_t ds = { dev, millisec };
    osEvent * result;

    result = (osEvent *)syscall(KERNEL_SYSCALL_SCHED_DEV_WAIT, &ds);

    if (result->status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    return *result;
}
#endif

/**
  * @}
  */

/**
  * @}
  */

