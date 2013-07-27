/**
 *******************************************************************************
 * @file    hal_core.h
 * @author  Olli Vanhoja
 * @brief   Hardware Abstraction Layer for the CPU core
 *******************************************************************************
 */

/** @addtogroup HAL
  * @{
  */

#pragma once
#ifndef HAL_CORE_H
#define HAL_CORE_H

#include <autoconf.h>
#include <kernel.h>

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
uint32_t syscall(uint32_t type, void * p);

/**
 * Test and set
 * @param lock pointer to the lock variable.
 */
int test_and_set(int * lock);

/* Core Implementation must declare following inlined functions:
 * + inline void eval_kernel_tick(void);
 * + inline void save_context(void);
 * + inline void load_context(void);
 * + inline void * rd_stack_ptr(void);
 * + inline void * rd_thread_stack_ptr(void);
 * + inline void wr_thread_stack_ptr(void * ptr);
 */

/* Select Core Implementation ************************************************/

#if configARM_PROFILE_M != 0 /* All M profile cores are handled in one file. */
#include "cortex_m.h"
#elif configARCH == __ARM4T__ /* ARM9 uses ARM4T arch */
#include "arm4t.h"
#elif configARCH == __ARM6__
#include "arm6.h"
#elif PU_TEST_BUILD == 1
#include "pu_test_core.h"
#else
    #error Selected ARM profile is not supported
#endif

#endif /* HAL_CORE_H */

/**
  * @}
  */

