/**
 *******************************************************************************
 * @file    kstring.h
 * @author  Olli Vanhoja
 * @brief   String routines
 *******************************************************************************
 */

#pragma once
#ifndef KSTRING_H
#define KSTRING_H

#include <stddef.h>
#include <stdint.h>

#ifndef PU_TEST_BUILD
void * memcpy(void * restrict destination, const void * source, size_t num);
//void * memmove(void * destination, const void * source, ksize_t num);
void * memset(void * ptr, int value, size_t num);
int strcmp(const char * str1, const char * str2);
int strncmp(const char * str1, const char * str2, size_t n);
char * strcpy(char * dst, const char * src);
char * strncpy(char * dst, const char * src, size_t n);
const char * strstr(const char * str1, const char * str2);
char * strnncat(char * dst, size_t ndst, const char * src, size_t nsrc);
size_t strlenn(const char * str, size_t max);
int uitoa32(char * str, uint32_t value);
int uitoah32(char * str, uint32_t value);
void ksprintf(char * str, size_t maxlen, const char * format, ...) __attribute__ ((format (printf, 3, 4)));
#endif

#endif /* KSTRING_H */
