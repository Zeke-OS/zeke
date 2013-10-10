/**
 *******************************************************************************
 * @file    kstring.h
 * @author  Olli Vanhoja
 * @brief   String routines
 *******************************************************************************
 */

#pragma once
#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void * memcpy(void * restrict destination, const void * source, size_t num);
//void * memmove(void * destination, const void * source, ksize_t num);
void * memset(void * ptr, int value, size_t num);
int strcmp(const char * str1, const char * str2);
char * strncpy(char * dst, const char * src, size_t n);
char * strnncat(char * dst, size_t ndst, char * src, size_t nsrc);

#endif /* STRING_H */
