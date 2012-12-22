/**
 *******************************************************************************
 * @file    sched.h
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler header.
 *******************************************************************************
 */

/** @addtogroup Scheduler
  * @{
  */

#pragma once
#ifndef SCHED_HPP
#define SCHED_HPP

#include "kernel.h"

#define SCHED_EXEC_FLAG     0x00000002u /*!< EXEC/SLEEP */
#define SCHED_IN_USE_FLAG   0x00000001u /*!< IN USE FLAG */

extern volatile uint32_t sched_enabled;
extern volatile uint32_t sched_cpu_load;

typedef struct {
    void * sp;              /*!< Stack pointer */
    int flags;              /*!< Status flags */
    osPriority priority;    /*!< Task priority */
    uint32_t uCounter;
} threadInfo_t;

/* Public function prototypes ------------------------------------------------*/
void sched_init(void);
void sched_start(void);
void sched_handler(void * st);
osStatus sched_threadDelay(uint32_t millisec);

#endif /* SCHED_HPP */

/**
  * @}
  */