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

#define configMCU_MODEL         MCU_MODEL_STM32F0
#define configSCHED_MAX_THREADS 10u     /*!< Maximum number of threads */
#define configSCHED_FREQ        100u    /*!< Scheduler frequency in Hz */
#define configSCHED_MAX_CYCLES  5u      /*!< Maximum cycles to prevent starvation of other threads */

#endif