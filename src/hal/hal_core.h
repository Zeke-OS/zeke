/**
 *************************************************************************$
 * @file    hal_core.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for core
 *************************************************************************$
 */

/** @addtogroup HAL
  * @{
  */

#pragma once
#ifndef HAL_CORE_H
#define HAL_CORE_H

#ifndef PU_TEST_BUILD
#include <intrinsics.h>
#endif

#include "kernel_config.h"

#ifndef KERNEL_CONFIG_H
#error kernel_config.h should be #included before hal_core.h
#endif

#include "kernel.h"

extern uint32_t flag_kernel_tick;

/**
 * Init hw stack frame
 * @param thread_def Thread definitions
 * @param argument Argument
 * @param a_del_thread Address of del_thread function
 */
void init_hw_stack_frame(osThreadDef_t * thread_def, void * argument, uint32_t a_del_thread);

/**
 * Make a system call from thread/user context
 * @param type Syscall type/code
 * @param p Syscall parameter(s)
 */
uint32_t syscall(int type, void * p);

/* Core Implementation must declare following inlined functions:
 * + inline void eval_kernel_tick(void);
 * + inline void save_context(void);
 * + inline void load_context(void);
 * + inline void * rd_stack_ptr(void);
 * + inline void * rd_thread_stack_ptr(void);
 * + inline void wr_thread_stack_ptr(void * ptr);
 */

/* Select Core Implementation ************************************************/

#ifdef __ARM_PROFILE_M__
#include "cortex_m.h"
#elif PU_TEST_BUILD == 1
#include "pu_test_core.h"
#else
    #error Selected ARM profile is not supported
#endif

#endif /* HAL_CORE_H */

/**
  * @}
  */

