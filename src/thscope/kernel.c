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

/** @addtogroup ThreadScope
  * @{
  */

#include "hal_core.h"
#include "syscalldef.h"
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
    syscall(SYSCALL_SCHED_GET_LOADAVG, loads);
}

/** @addtogroup Thread_Management
  * @{
  */

osThreadId osThreadCreate(osThreadDef_t * thread_def, void * argument)
{
    ds_osThreadCreate_t args = {
        .def      = thread_def,
        .argument = argument
    };
    osThreadId result;

    result = (osThreadId)syscall(SYSCALL_SCHED_THREAD_CREATE, &args);

    /* Request immediate context switch */
    req_context_switch();

    return result;
}

osThreadId osThreadGetId(void)
{
    return (osThreadId)syscall(SYSCALL_SCHED_THREAD_GETID, NULL);
}

osStatus osThreadTerminate(osThreadId thread_id)
{
    return (osStatus)syscall(SYSCALL_SCHED_THREAD_TERMINATE, &thread_id);
}

osStatus osThreadYield(void)
{
    /* Request immediate context switch */
    req_context_switch();

    return (osStatus)osOK;
}

osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority)
{
    ds_osSetPriority_t ds = {
        .thread_id = thread_id,
        .priority = priority
    };

    return (osStatus)syscall(SYSCALL_SCHED_THREAD_SETPRIORITY, &ds);
}

osPriority osThreadGetPriority(osThreadId thread_id)
{
    return (osPriority)syscall(SYSCALL_SCHED_THREAD_GETPRIORITY, &thread_id);
}

/**
  * @}
  */

/** @addtogroup Wait
  * @{
  */

/* Generic Wait Functions ****************************************************/

osStatus osDelay(uint32_t millisec)
{
    osStatus result;

    result = (osStatus)syscall(SYSCALL_SCHED_DELAY, &millisec);

    if (result != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    return result;
}

osEvent osWait(uint32_t millisec)
{
    osEvent result;

    result.status = (osStatus)syscall(SYSCALL_SCHED_WAIT, &millisec);

    if (result.status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    syscall(SYSCALL_SCHED_GET_EVENT, &result);
    return result;
}

/**
  * @}
  */

/** @addtogroup Signal
  * @{
  */

/* Signal Management *********************************************************/

int32_t osSignalSet(osThreadId thread_id, int32_t signal)
{
    ds_osSignal_t ds = {
        .thread_id = thread_id,
        .signal    = signal
    };

    return (int32_t)syscall(SYSCALL_SCHED_SIGNAL_SET, &ds);
}

int32_t osSignalClear(osThreadId thread_id, int32_t signal)
{
    ds_osSignal_t ds = {
        .thread_id = thread_id,
        .signal    = signal
    };

    return (int32_t)syscall(SYSCALL_SCHED_SIGNAL_CLEAR, &ds);
}

int32_t osSignalGetCurrent(void)
{
    return (int32_t)syscall(SYSCALL_SCHED_SIGNAL_GETCURR, NULL);
}

int32_t osSignalGet(osThreadId thread_id)
{
    return (int32_t)syscall(SYSCALL_SCHED_SIGNAL_GET, &thread_id);
}

osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
    ds_osSignalWait_t ds = {
        .signals  = signals,
        .millisec = millisec
    };
    osEvent result;

    result.status = (osStatus)syscall(SYSCALL_SCHED_SIGNAL_WAIT, &ds);

    if (result.status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    syscall(SYSCALL_SCHED_GET_EVENT, &result);
    return result;
}

/**
  * @}
  */

/** @addtogroup Mutex
  * @{
  */

/* Mutex Management ***********************************************************
 * NOTE: osMutexId/mutex_id is a direct pointer to a os_mutex_cb structure.
 */

/** TODO implement sleep strategy */

osMutex osMutexCreate(osMutexDef_t *mutex_def)
{
    mutex_cb_t cb = {
        .thread_id = -1,
        .lock      = 0,
        .strategy  = mutex_def->strategy
    };

    return cb;
}

osStatus osMutexWait(osMutex * mutex, uint32_t millisec)
{
    if (millisec != 0) {
        /** TODO Only spinlock is supported at the moment; implement timeout */
        return osErrorParameter;
    }

    while (syscall(SYSCALL_MUTEX_TEST_AND_SET, (void*)(&(mutex->lock)))) {
        /** TODO Should we lower the priority until lock is acquired
         *       osThreadGetPriority & osThreadSetPriority */
        /* Reschedule while waiting for lock */
        req_context_switch(); /* This should be done in user space */
    }

    mutex->thread_id = osThreadGetId();
    return osOK;
}

osStatus osMutexRelease(osMutex * mutex)
{
    if (mutex->thread_id == osThreadGetId()) {
        mutex->lock = 0;
        return osOK;
    }
    return osErrorResource;
}

/**
  * @}
  */

/** @addtogroup Dev_subsys
  * @{
  */

/* Non-CMSIS *****************************************************************/

#if configDEVSUBSYS != 0
int osDevOpen(osDev_t dev)
{
    return (int)syscall(SYSCALL_DEV_OPEN, &dev);
}

int osDevClose(osDev_t dev)
{
    return (int)syscall(SYSCALL_DEV_CLOSE, &dev);
}

int osDevCheckRes(osDev_t dev, osThreadId thread_id)
{
    ds_osDevHndl_t ds = {
        .dev       = dev,
        .thread_id = thread_id
    };

    return (int)syscall(SYSCALL_DEV_CHECK_RES, &ds);
}

int osDevCwrite(uint32_t ch, osDev_t dev)
{
    ds_osDevCData_t ds = {
        .dev  = dev,
        .data = &ch
    };

    return (int)syscall(SYSCALL_DEV_CWRITE, &ds);
}

int osDevCread(uint32_t * ch, osDev_t dev)
{
    ds_osDevCData_t ds = {
        .dev  = dev,
        .data = ch
    };

    return (int)syscall(SYSCALL_DEV_CREAD, &ds);
}

int osDevBwrite(const void * buff, size_t size, size_t count, osDev_t dev)
{
    ds_osDevBData_t ds = {
        .buff  = (void *)buff,
        .size  = size,
        .count = count,
        .dev   = dev
    };

    return (int)syscall(SYSCALL_DEV_BWRITE, &ds);
}

int osDevBread(void * buff, size_t size, size_t count, osDev_t dev)
{
    ds_osDevBData_t ds = {
        .buff  = buff,
        .size  = size,
        .count = count,
        .dev   = dev
    };

    return (int)syscall(SYSCALL_DEV_BWRITE, &ds);
}

int osDevBseek(int offset, int origin, size_t size, osDev_t dev)
{
    ds_osDevBSeekData_t ds = {
        .offset = offset,
        .origin = origin,
        .size   = size,
        .dev    = dev
    };

    return (int)syscall(SYSCALL_DEV_BSEEK, &ds);
}

osEvent osDevWait(osDev_t dev, uint32_t millisec)
{
    ds_osDevWait_t ds = {
        .dev      = dev,
        .millisec = millisec
    };
    osEvent result;

    result.status = (osStatus)syscall(SYSCALL_DEV_WAIT, &ds);

    if (result.status != osErrorResource) {
        /* Request context switch */
        req_context_switch();
    }

    /* Return a copy of the current state of the event structure */
    syscall(SYSCALL_SCHED_GET_EVENT, &result);
    return result;
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
