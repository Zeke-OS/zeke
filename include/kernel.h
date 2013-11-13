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

/* Commented out as Zeke's scope has been changed to only partial compatibility
 * with CMSIS specification.
 */
/* \note MUST REMAIN UNCHANGED: \b osCMSIS identifies the CMSIS-RTOS API version
 *#define osCMSIS           0x00003      ///< API version (main [31:16] .sub [15:0])*/

/// \note CAN BE CHANGED: \b osCMSIS_KERNEL identifies the underlaying RTOS kernel and version number.
#define osCMSIS_KERNEL    0x10000	   ///< RTOS identification and version (main [31:16] .sub [15:0])

#define osKernelSystemId "ZEKE   V1.00"   ///< RTOS identification string

/**
 * CMSIS-RTOS features supported.
 */
#define osFeature_MainThread   1       ///< main thread      1=main can be thread, 0=not available
#define osFeature_Pool         0       ///< Memory Pools:    1=available, 0=not available
#define osFeature_MailQ        0       ///< Mail Queues:     1=available, 0=not available
#define osFeature_MessageQ     0       ///< Message Queues:  1=available, 0=not available
#define osFeature_Signals      30      ///< maximum number of Signal Flags available per thread
#define osFeature_Semaphore    100     ///< maximum count for SemaphoreInit function
#define osFeature_Wait         1       ///< osWait function: 1=available, 0=not available

#include <stdint.h>
#include <stddef.h>
#include <types.h>
#include <pthread.h>
#include <mutex.h>
#include <semaphore.h>
#include <devtypes.h>
#include <autoconf.h>

#ifndef KERNEL_INTERNAL /* prevent kernel internals from implementing these
                         * as these should be implemented as syscall services.*/

/* ==== Non-CMSIS-RTOS functions ==== */
/**
 * Get load averages.
 * @param loads array where load averages will be stored.
 */
void osGetLoadAvg(uint32_t loads[3]);


//  ==== Kernel Control Functions ====

/// Check if the RTOS kernel is already started.
/// \return 0 RTOS is not started, 1 RTOS is started.
int32_t osKernelRunning(void);


//  ==== Generic Wait Functions ====

/// Wait for Timeout (Time Delay)
/// \param[in]     millisec      time delay value
/// \return status code that indicates the execution status of the function.
osStatus osDelay(uint32_t millisec);

#if (defined (osFeature_Wait)  &&  (osFeature_Wait != 0))     // Generic Wait available

/// Wait for Signal, Message, Mail, or Timeout
/// \param[in] millisec          timeout value or 0 in case of no time-out
/// \return event that contains signal, message, or mail information or error code.
osEvent osWait(uint32_t millisec);

#endif  // Generic Wait available


//  ==== Signal Management ====

/// Set the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signal        specifies the signal flags of the thread that should be set.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
int32_t osSignalSet(pthread_t thread_id, int32_t signal);

/// Clear the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signal        specifies the signal flags of the thread that shall be cleared.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
int32_t osSignalClear(pthread_t thread_id, int32_t signal);

/// Get Signal Flags status of the current thread.
/// \return previous signal flags of the current thread.
int32_t osSignalGetCurrent(void);

/// Get Signal Flags status of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
int32_t osSignalGet(pthread_t thread_id);

/// Wait for one or more Signal Flags to become signaled for the current \b RUNNING thread.
/// \param[in]     signals       wait until all specified signal flags set or 0 for any single signal flag.
/// \param[in]     millisec      timeout value or 0 in case of no time-out.
/// \return event flag information or error code.
osEvent osSignalWait(int32_t signals, uint32_t millisec);


//  ==== Mutex Management ====

/// Define a Mutex.
/// \param         name          name of the mutex object.
#define osMutexDef(name)  \
osMutexDef_t os_mutex_def_##name = { 0 }

/// Access a Mutex defintion.
/// \param         name          name of the mutex object.
#define osMutex(name)  \
&os_mutex_def_##name

/// Create and Initialize a Mutex object
/// \param[in]     mutex_def     mutex definition referenced with \ref osMutex.
/// \return mutex ID for reference by other functions or NULL in case of error.
osMutex osMutexCreate(osMutexDef_t * mutex_def);

/// Wait until a Mutex becomes available
/// \param[in]     mutex         mutex ID obtained by \ref osMutexCreate.
/// \param[in]     millisec      timeout value or 0 in case of no time-out.
/// \return status code that indicates the execution status of the function.
/// \note NON-CMSIS-RTOS IMPLEMENTATION
osStatus osMutexWait(osMutex * mutex, uint32_t millisec);

/// Release a Mutex that was obtained by \ref osMutexWait
/// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
/// \return status code that indicates the execution status of the function.
/// \note NON-CMSIS-RTOS IMPLEMENTATION
osStatus osMutexRelease(osMutex * mutex_id);

//  ==== Semaphore Management Functions ====

#if (defined (osFeature_Semaphore)  &&  (osFeature_Semaphore != 0))     // Semaphore available

/// Define a Semaphore object.
/// \param         name          name of the semaphore object.
/// \note CAN BE CHANGED: The parameter to \b osSemaphoreDef shall be consistent but the
///       macro body is implementation specific in every CMSIS-RTOS.
#define osSemaphoreDef(name)  \
osSemaphoreDef_t os_semaphore_def_##name = { 0 }

/// Access a Semaphore definition.
/// \param         name          name of the semaphore object.
/// \note CAN BE CHANGED: The parameter to \b osSemaphore shall be consistent but the
///       macro body is implementation specific in every CMSIS-RTOS.
#define osSemaphore(name)  \
&os_semaphore_def_##name

/// Create and Initialize a Semaphore object used for managing resources
/// \param[in]     semaphore_def semaphore definition referenced with \ref osSemaphore.
/// \param[in]     count         number of available resources.
/// \return semaphore ID for reference by other functions or NULL in case of error.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreCreate shall be consistent in every CMSIS-RTOS.
//osSemaphore osSemaphoreCreate(osSemaphoreDef_t * semaphore_def, int32_t count);

/// Wait until a Semaphore token becomes available
/// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphore.
/// \param[in]     millisec      timeout value or 0 in case of no time-out.
/// \return number of available tokens, or -1 in case of incorrect parameters.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreWait shall be consistent in every CMSIS-RTOS.
int32_t osSemaphoreWait(osSemaphore * semaphore, uint32_t millisec);

/// Release a Semaphore token
/// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphore.
/// \return status code that indicates the execution status of the function.
/// \note MUST REMAIN UNCHANGED: \b osSemaphoreRelease shall be consistent in every CMSIS-RTOS.
osStatus osSemaphoreRelease(osSemaphore * semaphore);

#endif     // Semaphore available


#endif /* KERNEL_INTERNAL */

#endif /* KERNEL_H */

/**
  * @}
  */
