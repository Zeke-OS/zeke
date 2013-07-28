/**
 *******************************************************************************
 * @file    ksignal.c
 * @author  Olli Vanhoja
 *
 * @brief   Source file for thread Signal Management in kernel.
 *
 *******************************************************************************
 */

#include <syscalldef.h>
#include <syscall.h>
#include "sched.h"
#include "timers.h"
#include "ksignal.h"

/** @addtogroup Signal
  * @{
  */

/**
 * Set signal and wakeup the thread.
 */
int32_t ksignal_threadSignalSet(osThreadId thread_id, int32_t signal)
{
    int32_t prev_signals;
    threadInfo_t * thread = sched_get_pThreadInfo(thread_id);

    if ((thread->flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000; /* Error code specified in CMSIS-RTOS */

    prev_signals = thread->signals;

    /* Update event struct */
    thread->event.value.signals = signal; /* Only this signal */
    thread->event.status = osEventSignal;

    if (((thread->flags & SCHED_NO_SIG_FLAG) == 0)
        && ((thread->sig_wait_mask & signal) != 0)) {
        ksignal_threadSignalWaitMaskClear(thread_id);

        /* Set the signaled thread back into execution */
        sched_thread_set_exec(thread_id);
    } else {
        /* Update signal flags if thread was not waiting for this signal */
        thread->signals |= signal; /* OR with all signals */
        /* However there is a slight chance that the thread misses the actual
         * signal event if some other thread sets another signal for the same
         * thread here. */
    }

    return prev_signals;
}

/**
 * Clear signal wait mask of a given thread
 */
void ksignal_threadSignalWaitMaskClear(osThreadId thread_id)
{
    threadInfo_t * thread = sched_get_pThreadInfo(thread_id);

    /* Release wait timeout timer */
    if (thread->wait_tim >= 0) {
        timers_release(thread->wait_tim);
    }

    /* Clear signal wait mask */
    current_thread->sig_wait_mask = 0;
}

/**
 * Clear selected signals
 * @param thread_id Thread id
 * @param signal    Signals to be cleared.
 */
int32_t ksignal_threadSignalClear(osThreadId thread_id, int32_t signal)
{
    int32_t prev_signals;
    threadInfo_t * thread = sched_get_pThreadInfo(thread_id);

    if ((thread->flags & SCHED_IN_USE_FLAG) == 0)
        return 0x80000000; /* Error code specified in CMSIS-RTOS */

    prev_signals = thread->signals;
    thread->signals &= ~(signal & 0x7fffffff);

    return prev_signals;
}

/**
 * Get signals of the currently running thread
 * @return Signals
 */
int32_t ksignal_threadSignalGetCurrent(void)
{
    return current_thread->signals;
}

/**
 * Get signals of a thread
 * @param thread_id Thread id.
 * @return          Signals of the selected thread.
 */
int32_t ksignal_threadSignalGet(osThreadId thread_id)
{
    threadInfo_t * thread = sched_get_pThreadInfo(thread_id);
    return thread->signals;
}

/**
 * Wait for a signal(s)
 * @param signals   Signals that can woke-up the suspended thread.
 * @millisec        Timeout if selected signal is not invoken.
 * @return          osStatus.
 */
osStatus ksignal_threadSignalWait(int32_t signals, uint32_t millisec)
{
    int tim;
    osStatus status = osOK;

    /* Event status is now timeout but will be changed if any event occurs
     * as event is returned as a pointer. */
    current_thread->event.status = osEventTimeout;

    if (millisec != osWaitForever) {
        if ((tim = timers_add(current_thread->id, TIMERS_FLAG_ENABLED, millisec)) < 0) {
            /* Timer error will be most likely but not necessarily returned
             * as is. There is however a slight chance of event triggering
             * before control returns back to this thread. It is completely OK
             * to clear this error if that happens. */
            current_thread->event.status = osErrorResource;
            status = osErrorResource;
        }
        current_thread->wait_tim = tim;
    }

    if (current_thread->event.status != osErrorResource) {
        current_thread->sig_wait_mask = signals;
        sched_thread_sleep_current();
    }

    return status;
}

/* Syscall handlers ***********************************************************/
/** @addtogroup Syscall_handlers
  * @{
  */


/**
 * Scheduler signal syscall handler
 * @param type Syscall type
 * @param p Syscall parameters
 */
uint32_t ksignal_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_SIGNAL_SET:
        return (uint32_t)ksignal_threadSignalSet(
                    ((ds_osSignal_t *)p)->thread_id,
                    ((ds_osSignal_t *)p)->signal
                );

    case SYSCALL_SIGNAL_CLEAR:
        return (uint32_t)ksignal_threadSignalClear(
                    ((ds_osSignal_t *)p)->thread_id,
                    ((ds_osSignal_t *)p)->signal
                );

    case SYSCALL_SIGNAL_GETCURR:
        return (uint32_t)ksignal_threadSignalGetCurrent();

    case SYSCALL_SIGNAL_GET:
        return (uint32_t)ksignal_threadSignalGet(
                    *((osThreadId *)p)
                );

    case SYSCALL_SIGNAL_WAIT:
        return (uint32_t)ksignal_threadSignalWait(
                    ((ds_osSignalWait_t *)p)->signals,
                    ((ds_osSignalWait_t *)p)->millisec
                );

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */

/**
  * @}
  */

