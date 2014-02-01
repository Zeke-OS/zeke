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

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <pthread.h>
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
