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
#define MCU_MODEL_STR912F       2

/* Supported load average calculation periods */
#define SCHED_LAVGPERIOD_5SEC   5
#define SCHED_LAVGPERIOD_11SEC  11

/* Kernel configuration ******************************************************/
#define configMCU_MODEL         MCU_MODEL_STM32F0

/* Scheduler */
#define configSCHED_MAX_THREADS 10      /*!< Maximum number of threads */
#define configFAST_FORK         0       /*!< Use a queue to find the next free threadId. */
#define configSCHED_FREQ        100u    /*!< Scheduler frequency in Hz */
#define configSCHED_LAVG_PER    SCHED_LAVGPERIOD_11SEC
#define configHEAP_BOUNDS_CHECK 0       /*!< Enable or disable heap bounds check */
#define configTIMERS_MAX        4       /*!< Maximum number of timers available */

/* APP Main */
#define configAPP_MAIN_SSIZE    200u    /*!< Stack size for app_main thread */
#define configAPP_MAIN_PRI      osPriorityNormal /*!< Priority of app_main thread */

/* Kernel Services */
#define configDEVSUBSYS         1       /*!< Enable device driver subsystem */
#define configPTTK91_VM         0       /*!< PTTK91 kernel virtual machine */
/* End of Kernel configuration ***********************************************/

#endif
