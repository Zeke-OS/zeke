/**
 *******************************************************************************
 * @file    locks.h
 * @author  Olli Vanhoja
 * @brief   Header file for locks.c module
 *
 *******************************************************************************
 */

#pragma once
#ifndef LOCKS_H
#define LOCKS_H

#include <stdint.h>

uint32_t locks_syscall(uint32_t type, void * p);
void locks_semaphore_v(uint32_t * s);
int locks_semaphore_p(uint32_t * s);

#endif /* LOCKS_H */

