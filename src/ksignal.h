/**
 *******************************************************************************
 * @file    ksignal.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for thread Signal Management in kernel (ksignal.c).
 *
 *******************************************************************************
 */

#pragma once
#ifndef KSIGNAL_H
#define KSIGNAL_H

#include <stdint.h>
#include "kernel.h"

int32_t ksignal_threadSignalSet(osThreadId thread_id, int32_t signal);
void ksignal_threadSignalWaitMaskClear(osThreadId thread_id);
int32_t ksignal_threadSignalClear(osThreadId thread_id, int32_t signal);
int32_t ksignal_threadSignalGetCurrent(void);
int32_t ksignal_threadSignalGet(osThreadId thread_id);
osStatus ksignal_threadSignalWait(int32_t signals, uint32_t millisec);
uint32_t ksignal_syscall(uint32_t type, void * p);

#endif /* KSIGNAL_H */

