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

