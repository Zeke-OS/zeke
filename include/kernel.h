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
typedef enum  {
  osPriorityIdle          = -3,          ///< priority: idle (lowest)
  osPriorityLow           = -2,          ///< priority: low
  osPriorityBelowNormal   = -1,          ///< priority: below normal
  osPriorityNormal        =  0,          ///< priority: normal (default)
  osPriorityAboveNormal   = +1,          ///< priority: above normal
  osPriorityHigh          = +2,          ///< priority: high
  osPriorityRealtime      = +3,          ///< priority: realtime (highest)
  osPriorityError         =  0x84        ///< system cannot determine priority or thread has illegal priority
} osPriority;

/// Entry point of a thread.
/// \note MUST REMAIN UNCHANGED: \b os_pthread shall be consistent in every CMSIS-RTOS.
typedef void (*os_pthread) (void const *argument);



/**
  * Thread enters to sleep
  * @note This doesn't change EXEC flag of the thread so thread will continue
  *       imeadiately after other thread have used their time slot.
  * @todo Sleet doesnt happen immediately which could be a cause of some problems
  */
#define kernel_ThreadSleep() SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk // Set PendSV pending status

/* Following functions are declared in kernel.c */
void kernel_init(void);
void kernel_start(void);

/* Following functions are declared in sched.c */
int kernel_ThreadCreate(void (*p)(void*), void const * arg, void * stackAddr, size_t stackSize);
void kernel_ThreadSleep_ms(uint32_t delay);
void kernel_ThreadWait(void);
void kernel_ThreadNotify(int thread_id);

#endif /* KERNEL_HPP */

/**
  * @}
  */