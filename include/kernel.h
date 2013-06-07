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
#include "mutex.h"
#include "semaphore.h"
#include "devtypes.h"
#include "kernel_config.h"

// ==== Enumeration, structures, defines ====

/**
 * Priority used for thread control.
 */
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

/**
 * Timeout value
 */
#define osWaitForever     0xFFFFFFFFu       /*!< wait forever timeout value */

/// Status code values returned by CMSIS-RTOS functions
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

/// Timer type value for the timer definition
typedef enum  {
  osTimerOnce             =     0,       ///< one-shot timer
  osTimerPeriodic         =     1        ///< repeating timer
} os_timer_type;

/// Entry point of a thread.
typedef void (*os_pthread) (void const * argument);

// >>> the following data type definitions may shall adapted towards a specific RTOS

/// Thread ID identifies the thread (pointer to a thread control block).
typedef int osThreadId;

/// Timer ID identifies the timer (pointer to a timer control block).
typedef int osTimerId;

/**
 * Mutex cb mutex.
 * @note All data related to the mutex is stored in user space structure and
 *       it is DANGEROUS to edit its contents in thread context.
 */
typedef struct os_mutex_cb osMutex;

/**
 * Semaphore ID identifies the semaphore (pointer to a semaphore control block).
 * @note All data related to the mutex is stored in user space structure and
 *       it is DANGEROUS to edit its contents in thread context.
 */
typedef struct os_semaphore_cb osSemaphore;

/// Message ID identifies the message queue (pointer to a message queue control block).
typedef struct os_messageQ_cb *osMessageQId;

/// Mail ID identifies the mail queue (pointer to a mail queue control block).
typedef struct os_mailQ_cb *osMailQId;

/// Thread Definition structure contains startup information of a thread.
typedef const struct os_thread_def {
    os_pthread  pthread;    ///< start address of thread function
    osPriority  tpriority;  ///< initial thread priority
    void *      stackAddr;  ///< Stack address
    size_t      stackSize;  ///< Size of stack reserved for the thread.
} osThreadDef_t;

/// Mutex Definition structure contains setup information for a mutex.
typedef const struct os_mutex_def  {
    enum os_mutex_strategy strategy;
} osMutexDef_t;

/// Event structure contains detailed information about an event.
typedef struct  {
    osStatus                 status;    ///< status code: event or error information
    union  {
        uint32_t                    v;  ///< message as 32-bit value
        void                       *p;  ///< message or mail as void pointer
        int32_t               signals;  ///< signal flags
    } value;                            ///< event value
    union  {
        osMailQId             mail_id;  ///< mail id obtained by \ref osMailCreate
        osMessageQId       message_id;  ///< message id obtained by \ref osMessageCreate
    } def;                              ///< event definition
} osEvent;


#ifndef KERNEL_INTERNAL /* prevent kernel internals from implementing these
                         * as these should be implemented as syscall services.*/

/* ==== Non-CMSIS-RTOS functions ==== */
#if configDEVSUBSYS != 0
/**
 * Open and lock device access.
 * @param dev the device accessed.
 * @return DEV_OERR_OK (0) if the lock acquired and device access was opened;
 * otherwise DEV_OERR_x
 */
int osDevOpen(osDev_t dev);

/**
 * Close and release device access.
 * @param dev the device.
 * @return DEV_CERR_OK (0) if the device access was closed succesfully;
 * otherwise DEV_CERR_x.
 */
int osDevClose(osDev_t dev);

/**
 * Check if thread_id has locked the device given in dev.
 * @param dev device that is checked for lock.
 * @param thread_id thread that may have the lock for the dev.
 * @return 0 if the device is not locked by thread_id.
 */
int osDevCheckRes(osDev_t dev, osThreadId thread_id);

/**
 * Write to a character device.
 * @param ch is written to the device.
 * @param dev device.
 * @return Error code.
 */
int osDevCwrite(uint32_t ch, osDev_t dev);

/**
 * Read from a character device.
 * @param ch output is written here.
 * @param dev device.
 * @return Error code.
 */
int osDevCread(uint32_t * ch, osDev_t dev);

/**
 * Write to a block device.
 * @param buff Pointer to the array of elements to be written.
 * @param size in bytes of each element to be written.
 * @param count number of elements, each one with a size of size bytes.
 * @param dev device to be written to.
 * @return Error code.
 */
int osDevBwrite(const void * buff, size_t size, size_t count, osDev_t dev);

/**
 * Read from a block device.
 * @param buff pointer to a block of memory with a size of at least (size*count) bytes, converted to a void *.
 * @param size in bytes, of each element to be read.
 * @param count number of elements, each one with a size of size bytes.
 * @param dev device to be read from.
 * @return Error code.
 */
int osDevBread(void * buff, size_t size, size_t count, osDev_t dev);

/**
 * Seek block device.
 * TODO Implementation
 * @param Number of size units to offset from origin.
 * @param origin Position used as reference for the offset.
 * @param size in bytes, of each element.
 * @param dev device to be seeked from.
 * @return Error code.
 */
int osDevBseek(int offset, int origin, size_t size, osDev_t dev);

/**
 * Wait for device.
 * @param dev the device accessed.
 * @param millisec timeout.
 */
osEvent osDevWait(osDev_t dev, uint32_t millisec);
#endif

/**
 * Get load averages.
 * @param loads array where load averages will be stored.
 */
void osGetLoadAvg(uint32_t loads[3]);


//  ==== Kernel Control Functions ====

/// Check if the RTOS kernel is already started.
/// \return 0 RTOS is not started, 1 RTOS is started.
int32_t osKernelRunning(void);


//  ==== Thread Management ====

/// Create a Thread Definition with function, priority, and stack requirements.
/// \param          name        name of the thread function.
/// \param          priority    initial priority of the thread function.
/// \param          stackaddr   Stack address.
/// \param          stacksz     stack size.
#if defined (osObjectsExternal)  // object is external
#define osThreadDef(name, priority, stackaddr, stacksz)  \
extern osThreadDef_t os_thread_def_##name
#else                            // define the object
#define osThreadDef(name, priority, stackaddr, stacksz)  \
osThreadDef_t os_thread_def_##name = \
{ (name), (priority), (stackaddr), (stacksz) }
#endif

/// Create a thread and add it to Active Threads and set it to state READY.
/// \param[in]     thread_def    thread definition.
/// \param[in]     argument      pointer that is passed to the thread function as start argument.
/// \return thread ID for reference by other functions or NULL in case of error.
osThreadId osThreadCreate(osThreadDef_t * thread_def, void * argument);

/// Return the thread ID of the current running thread.
/// \return thread ID for reference by other functions or NULL in case of error.
osThreadId osThreadGetId(void);

/// Terminate execution of a thread and remove it from Active Threads.
/// \param[in]     thread_id   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return status code that indicates the execution status of the function.
osStatus osThreadTerminate(osThreadId thread_id);

/// Pass control to next thread that is in state \b READY.
/// \return status code that indicates the execution status of the function.
osStatus osThreadYield(void);

/// Change priority of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     priority      new priority value for the thread function.
/// \return status code that indicates the execution status of the function.
osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority);

/// Get current priority of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return current priority value of the thread function.
osPriority osThreadGetPriority(osThreadId thread_id);


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
int32_t osSignalSet(osThreadId thread_id, int32_t signal);

/// Clear the specified Signal Flags of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \param[in]     signal        specifies the signal flags of the thread that shall be cleared.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
int32_t osSignalClear(osThreadId thread_id, int32_t signal);

/// Get Signal Flags status of the current thread.
/// \return previous signal flags of the current thread.
int32_t osSignalGetCurrent(void);

/// Get Signal Flags status of an active thread.
/// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return previous signal flags of the specified thread or 0x80000000 in case of incorrect parameters.
int32_t osSignalGet(osThreadId thread_id);

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
