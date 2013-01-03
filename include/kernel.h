/**
 *******************************************************************************
 * @file    kernel.h
 * @author  Olli Vanhoja
 * @brief   Kernel header
 *******************************************************************************
 */

/** @addtogroup Kernel
  * @{
  */

#pragma once
#ifndef KERNEL_H
#define KERNEL_H

/// \note MUST REMAIN UNCHANGED: \b osCMSIS identifies the CMSIS-RTOS API version
#define osCMSIS           0x00003      ///< API version (main [31:16] .sub [15:0])

/// \note CAN BE CHANGED: \b osCMSIS_KERNEL identifies the underlaying RTOS kernel and version number.
#define osCMSIS_KERNEL    0x10000	   ///< RTOS identification and version (main [31:16] .sub [15:0])

/// \note MUST REMAIN UNCHANGED: \b osKernelSystemId shall be consistent in every CMSIS-RTOS.
#define osKernelSystemId "KERNEL V1.00"   ///< RTOS identification string

/// \note MUST REMAIN UNCHANGED: \b osFeature_xxx shall be consistent in every CMSIS-RTOS.
#define osFeature_MainThread   1       ///< main thread      1=main can be thread, 0=not available
#define osFeature_Pool         0       ///< Memory Pools:    1=available, 0=not available
#define osFeature_MailQ        0       ///< Mail Queues:     1=available, 0=not available
#define osFeature_MessageQ     0       ///< Message Queues:  1=available, 0=not available
#define osFeature_Signals      31      ///< maximum number of Signal Flags available per thread
#define osFeature_Semaphore    0       ///< maximum count for SemaphoreInit function
#define osFeature_Wait         1       ///< osWait function: 1=available, 0=not available

#include <stdint.h>
#include <stddef.h>

#ifndef PU_TEST_BUILD
#include "stm32f0xx.h"
#endif
#include "kernel_config.h"

// ==== Enumeration, structures, defines ====

/// Priority used for thread control.
/// \note MUST REMAIN UNCHANGED: \b osPriority shall be consistent in every CMSIS-RTOS.
typedef enum {
    osPriorityIdle          = -3,       ///< priority: idle (lowest)
    osPriorityLow           = -2,       ///< priority: low
    osPriorityBelowNormal   = -1,       ///< priority: below normal
    osPriorityNormal        =  0,       ///< priority: normal (default)
    osPriorityAboveNormal   = +1,       ///< priority: above normal
    osPriorityHigh          = +2,       ///< priority: high
    osPriorityRealtime      = +3,       ///< priority: realtime (highest)
    osPriorityError         =  0x84     ///< system cannot determine priority or thread has illegal priority
} osPriority;

/// Timeout value
/// \note MUST REMAIN UNCHANGED: \b osWaitForever shall be consistent in every CMSIS-RTOS.
#define osWaitForever     0xFFFFFFFFu       ///< wait forever timeout value

/// Status code values returned by CMSIS-RTOS functions
/// \note MUST REMAIN UNCHANGED: \b osStatus shall be consistent in every CMSIS-RTOS.
typedef enum  {
    osOK                    =     0,        ///< function completed; no event occurred.
    osEventSignal           =  0x08,        ///< function completed; signal event occurred.
    osEventMessage          =  0x10,        ///< function completed; message event occurred.
    osEventMail             =  0x20,        ///< function completed; mail event occurred.
    osEventTimeout          =  0x40,        ///< function completed; timeout occurred.
    osErrorParameter        =  0x80,        ///< parameter error: a mandatory parameter was missing or specified an incorrect object.
    osErrorResource         =  0x81,        ///< resource not available: a specified resource was not available.
    osErrorTimeoutResource  =  0xC1,        ///< resource not available within given time: a specified resource was not available within the timeout period.
    osErrorISR              =  0x82,        ///< not allowed in ISR context: the function cannot be called from interrupt service routines.
    osErrorISRRecursive     =  0x83,        ///< function called multiple times from ISR with same object.
    osErrorPriority         =  0x84,        ///< system cannot determine priority or thread has illegal priority.
    osErrorNoMemory         =  0x85,        ///< system is out of memory: it was impossible to allocate or reserve memory for the operation.
    osErrorValue            =  0x86,        ///< value of a parameter is out of range.
    osErrorOS               =  0xFF,        ///< unspecified RTOS error: run-time error but no other error message fits.
    os_status_reserved      =  0x7FFFFFFF   ///< prevent from enum down-size compiler optimization.
} osStatus;

/// Entry point of a thread.
/// \note MUST REMAIN UNCHANGED: \b os_pthread shall be consistent in every CMSIS-RTOS.
typedef void (*os_pthread) (void const * argument);

// >>> the following data type definitions may shall adapted towards a specific RTOS

/// Thread ID identifies the thread (pointer to a thread control block).
/// \note CAN BE CHANGED: \b os_thread_cb is implementation specific in every CMSIS-RTOS.
typedef int osThreadId;

/// Message ID identifies the message queue (pointer to a message queue control block).
/// \note CAN BE CHANGED: \b os_messageQ_cb is implementation specific in every CMSIS-RTOS.
typedef struct os_messageQ_cb *osMessageQId;

/// Mail ID identifies the mail queue (pointer to a mail queue control block).
/// \note CAN BE CHANGED: \b os_mailQ_cb is implementation specific in every CMSIS-RTOS.
typedef struct os_mailQ_cb *osMailQId;

/// Thread Definition structure contains startup information of a thread.
/// \note CAN BE CHANGED: \b os_thread_def is implementation specific in every CMSIS-RTOS.
typedef const struct os_thread_def {
    os_pthread  pthread;    ///< start address of thread function
    osPriority  tpriority;  ///< initial thread priority
    void *      stackAddr;  ///< Stack address
    size_t      stackSize;  ///< Size of stack reserved for the thread. (CMSIS-RTOS: stack size requirements in bytes; 0 is default stack size)
} osThreadDef_t;

/// Event structure contains detailed information about an event.
/// \note MUST REMAIN UNCHANGED: \b os_event shall be consistent in every CMSIS-RTOS.
///       However the struct may be extended at the end.
typedef struct  {
  osStatus                 status;     ///< status code: event or error information
  union  {
    uint32_t                    v;     ///< message as 32-bit value
    void                       *p;     ///< message or mail as void pointer
    int32_t               signals;     ///< signal flags
  } value;                             ///< event value
  union  {
    osMailQId             mail_id;     ///< mail id obtained by \ref osMailCreate
    osMessageQId       message_id;     ///< message id obtained by \ref osMessageCreate
  } def;                               ///< event definition
} osEvent;


#ifndef KERNEL_INTERNAL /* prevent kernel internals from implementing these
                         * as these should be implemented as syscall services.*/

/* ==== Non-CMSIS-RTOS functions ==== */
/* none atm */



//  ==== Kernel Control Functions ====

/// Check if the RTOS kernel is already started.
/// \note MUST REMAIN UNCHANGED: \b osKernelRunning shall be consistent in every CMSIS-RTOS.
/// \return 0 RTOS is not started, 1 RTOS is started.
int32_t osKernelRunning(void);



//  ==== Thread Management ====

/// Create a Thread Definition with function, priority, and stack requirements.
/// \param          name        name of the thread function.
/// \param          priority    initial priority of the thread function.
/// \param          stackaddr   Stack address.
/// \param          stacksz     stack size.
/// \note CAN BE CHANGED: The parameters to \b osThreadDef shall be consistent but the
///       macro body is implementation specific in every CMSIS-RTOS.
#if defined (osObjectsExternal)  // object is external
#define osThreadDef(name, priority, stackaddr, stacksz)  \
extern osThreadDef_t os_thread_def_##name
#else                            // define the object
#define osThreadDef(name, priority, stackaddr, stacksz)  \
osThreadDef_t os_thread_def_##name = \
{ (name), (priority), (stackaddr), (stacksz) }
#endif

/// Create a thread and add it to Active Threads and set it to state READY.
/// \param[in]     thread_def    thread definition referenced with \ref osThread.
/// \param[in]     argument      pointer that is passed to the thread function as start argument.
/// \return thread ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osThreadCreate shall be consistent in every CMSIS-RTOS.
osThreadId osThreadCreate(osThreadDef_t * thread_def, void * argument);

/// Return the thread ID of the current running thread.
/// \return thread ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osThreadGetId shall be consistent in every CMSIS-RTOS.
osThreadId osThreadGetId(void);

/// Terminate execution of a thread and remove it from Active Threads.
/// \param[in]     thread_id   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osThreadTerminate shall be consistent in every CMSIS-RTOS.
osStatus osThreadTerminate(osThreadId thread_id);

/// Pass control to next thread that is in state \b READY.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osThreadYield shall be consistent in every CMSIS-RTOS.
osStatus osThreadYield(void);



//  ==== Generic Wait Functions ====

/// Wait for Timeout (Time Delay)
/// \param[in]     millisec      time delay value
/// \return status code that indicates the execution status of the function.
osStatus osDelay(uint32_t millisec);

#if (defined (osFeature_Wait)  &&  (osFeature_Wait != 0))     // Generic Wait available

/// Wait for Signal, Message, Mail, or Timeout
/// \param[in] millisec          timeout value or 0 in case of no time-out
/// \return event that contains signal, message, or mail information or error code.
/// \note MUST REMAIN UNCHANGED: \b osWait shall be consistent in every CMSIS-RTOS.
osEvent osWait(uint32_t millisec);

#endif  // Generic Wait available



//  ==== Signal Management ====

/// Set the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signals       specifies the signal flags of the thread that should be set.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
/// \note MUST REMAIN UNCHANGED: \b osSignalSet shall be consistent in every CMSIS-RTOS.
int32_t osSignalSet(osThreadId thread_id, int32_t signal);

/// Clear the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signals       specifies the signal flags of the thread that shall be cleared.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
/// \note MUST REMAIN UNCHANGED: \b osSignalClear shall be consistent in every CMSIS-RTOS.
int32_t osSignalClear(osThreadId thread_id, int32_t signal);

/// Get Signal Flags status of the current thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
int32_t osSignalGetCurrent(void);

/// Get Signal Flags status of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
/// \note MUST REMAIN UNCHANGED: \b osSignalGet shall be consistent in every CMSIS-RTOS.
int32_t osSignalGet(osThreadId thread_id);

/// Wait for one or more Signal Flags to become signaled for the current \b RUNNING thread.
/// \param[in]     signals       wait until all specified signal flags set or 0 for any single signal flag.
/// \param[in]     millisec      timeout value or 0 in case of no time-out.
/// \return event flag information or error code.
/// \note MUST REMAIN UNCHANGED: \b osSignalWait shall be consistent in every CMSIS-RTOS.
osEvent osSignalWait(int32_t signals, uint32_t millisec);

#endif /* KERNEL_INTERNAL */

#endif /* KERNEL_H */

/**
  * @}
  */
