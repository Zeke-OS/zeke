/**
 *******************************************************************************
 * @file    mutex.c
 * @author  Olli Vanhoja
 * @brief   Zero Kernel user space code
 *
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#include "hal_core.h"
#include "hal_mcu.h"
#include "syscall.h"
#include "kernel.h"

/** @addtogroup Mutex
  * @{
  */

/* Mutex Management ***********************************************************
 * NOTE: osMutexId/mutex_id is a direct pointer to a os_mutex_cb structure.
 */

/** TODO implement sleep strategy */

osMutexCreate(osMutexDef_t *mutex_def)
{
    mutex_cb_t cb;

    cb.thread_id = -1;
    cb.lock = 0;
    cb.strategy = mutex_def->strategy;

    return cb;
}

osStatus osMutexWait(osMutexId mutex_id, uint32_t millisec)
{
    if (millisec != 0) {
        /** TODO Only spinlock is supported at the moment; implement timeout */
        return osErrorParameter;
    }

    while (syscall(KERNEL_SYSCALL_TEST_AND_SET, mutex_od.lock)) {
        /** TODO Should we lower the priority until lock is acquired
         *       osThreadGetPriority & osThreadSetPriority */
        /* Reschedule while waiting for lock */
        req_context_switch(); /* This should be done in user space */
    }

    mutex_id->thread_id = osThreadGetId();
    return osOK;
}

osStatus osMutexRelease(osMutexId mutex_id)
{
    if (mutex_id->thread_id == osThreadGetId()) {
        mutex_id->lock = 0;
        return osOK;
    }
    return osErrorResource;
}

/**
  * @}
  */

/**
  * @}
  */

