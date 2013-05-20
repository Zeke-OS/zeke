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

#endif /* LOCKS_H */

