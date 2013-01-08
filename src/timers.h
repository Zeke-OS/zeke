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
int timers_add(int thread_id, uint32_t millisec);
void timers_release(int thread_id);

#endif /* TIMERS_H */
