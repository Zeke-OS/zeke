/**
 *******************************************************************************
 * @file    timers.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for kernel timers (timers.c).
 *
 *******************************************************************************
 */

#pragma once
#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include "kernel.h"

extern uint32_t timers_value;

void timers_init(void);
void timers_run(void);

/**
 * Reserve a new timer
 * @return Reference to a timer or -1 if allocation failed.
 */
int timers_add(osThreadId thread_id, os_timer_type type, uint32_t millisec);

void timers_start(int tim);
void timers_release(int tim);
osThreadId timers_get_owner(int tim);

/**
 * Get owner of the timer
 * @param tim timer id
 * @return thread id or -1 if out of bounds or timer is currently in released
 *         state
 */
osThreadId timers_get_owner(int tim);

#endif /* TIMERS_H */
