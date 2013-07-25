/**
 *******************************************************************************
 * @file    string.h
 * @author  Olli Vanhoja
 * @brief   String routines
 *******************************************************************************
 */

#pragma once
#ifndef STRING_H
#define STRING_H

#include <stdint.h>

typedef uint32_t ksize_t;

void * memcpy(void * restrict destination, const void * source, ksize_t num);
//void * memmove(void * destination, const void * source, ksize_t num);
void * memset(void * ptr, int value, ksize_t num);

#endif /* STRING_H */
