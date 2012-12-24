/**
 *******************************************************************************
 * @file    kernel_config.h
 * @author  Olli Vanhoja
 *
 * @brief   Configuration defintions for kernel.
 *
 *******************************************************************************
 */

#pragma once
#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

/* Supported MCU models */
#define MCU_MODEL_STM32F0       1

/* Kernel configuration */
#define configMCU_MODEL         MCU_MODEL_STM32F0
#define configSCHED_MAX_THREADS 10u     /*!< Maximum number of threads */
#define configSCHED_FREQ        100u    /*!< Scheduler frequency in Hz */
#define configSCHED_MAX_SLICES  4u      /*!< Maximum number of time slices for
                                         *   normal priority thread.
                                         *
                                         *   Atfer all slices are used the
                                         *   thread is preempted and context
                                         *   switch will be enforced.
                                         *   This value is  scaled for all
                                         *   priorities.
                                         *
                                         *    if pri < 0:
                                         *        scaled = max_slices / (|pri| + 1)
                                         *    if pri >= 0
                                         *        scaled = max_slices * pri */
#define configAPP_MAIN_SSIZE    200u    /*!< Stack size for app_main thread */
#define configAPP_MAIN_PRI      osPriorityNormal /*!< Priority of app_main thread */

#endif
