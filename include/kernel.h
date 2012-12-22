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
#ifndef KERNEL_HPP
#define KERNEL_HPP

/// \note MUST REMAIN UNCHANGED: \b osCMSIS identifies the CMSIS-RTOS API version
#define osCMSIS           0x00003      ///< API version (main [31:16] .sub [15:0])

/// \note CAN BE CHANGED: \b osCMSIS_KERNEL identifies the underlaying RTOS kernel and version number.
#define osCMSIS_KERNEL    0x10000	   ///< RTOS identification and version (main [31:16] .sub [15:0])

/// \note MUST REMAIN UNCHANGED: \b osKernelSystemId shall be consistent in every CMSIS-RTOS.
#define osKernelSystemId "KERNEL V1.00"   ///< RTOS identification string

/// \note MUST REMAIN UNCHANGED: \b osFeature_xxx shall be consistent in every CMSIS-RTOS.
#define osFeature_MainThread   0       ///< main thread      1=main can be thread, 0=not available
#define osFeature_Pool         0       ///< Memory Pools:    1=available, 0=not available
#define osFeature_MailQ        0       ///< Mail Queues:     1=available, 0=not available
#define osFeature_MessageQ     0       ///< Message Queues:  1=available, 0=not available
#define osFeature_Signals      8       ///< maximum number of Signal Flags available per thread
#define osFeature_Semaphore    0       ///< maximum count for SemaphoreInit function
#define osFeature_Wait         1       ///< osWait function: 1=available, 0=not available

#include <stdint.h>
#include <stddef.h>

#include "stm32f0xx.h"
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

/// Thread Definition structure contains startup information of a thread.
/// \note CAN BE CHANGED: \b os_thread_def is implementation specific in every CMSIS-RTOS.
typedef const struct os_thread_def {
    os_pthread  pthread;    ///< start address of thread function
    osPriority  tpriority;  ///< initial thread priority
    void *      stackAddr;  ///< Stack address
    size_t      stackSize;  ///< Size of stack reserved for the thread. (CMSIS-RTOS: stack size requirements in bytes; 0 is default stack size)
    void *      argument;
} osThreadDef_t;


#ifndef KERNEL_INTERNAL /* prevent kernel internals from implementing these
                         * as these should be implemented as syscall services.*/

/* ==== Non-CMSIS-RTOS ==== */
void kernel_init(void);
void kernel_start(void);

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
/// \note Doesn't fulfill CMSIS-RTOS specification as return value is int here.
int osThreadCreate(osThreadDef_t * thread_def, void * argument);


//  ==== Generic Wait Functions ====

/// Wait for Timeout (Time Delay)
/// \param[in]     millisec      time delay value
/// \return status code that indicates the execution status of the function.
osStatus osDelay(uint32_t millisec);

#if (defined (osFeature_Wait)  &&  (osFeature_Wait != 0))     // Generic Wait available

/// Wait for Signal, Message, Mail, or Timeout
/// \param[in] millisec          timeout value or 0 in case of no time-out
/// \return event that contains signal, message, or mail information or error code.
/// \note Doesn't fulfill CMSIS-RTOS specification as return value is different.
osStatus osWait(uint32_t millisec);

#endif  // Generic Wait available

#endif /* KERNEL_INTERNAL */

#endif /* KERNEL_HPP */

/**
  * @}
  */