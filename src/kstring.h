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

#define _TO_STR(x) #x
#define TO_STR(x) _TO_STR(x)

#ifndef PU_TEST_BUILD
void * memcpy(void * restrict destination, const void * source, size_t num);
void * memmove(void * destination, const void * source, size_t num);
void * memset(void * ptr, int value, size_t num);
int strcmp(const char * str1, const char * str2);
int strncmp(const char * str1, const char * str2, size_t n);
char * strcpy(char * dst, const char * src);
char * strncpy(char * dst, const char * src, size_t n);
size_t strlcpy(char * dst, const char * src, size_t siz);
const char * strstr(const char * str1, const char * str2);
#endif
size_t strlenn(const char * str, size_t max);
char * strnncat(char * dst, size_t ndst, const char * src, size_t nsrc);
int strvalid(const char * buf, size_t len);
int uitoa32(char * str, uint32_t value);
int uitoah32(char * str, uint32_t value);
char * kstrdup(const char * src, size_t max);
void ksprintf(char * str, size_t maxlen, const char * format, ...) __attribute__ ((format (printf, 3, 4)));

#endif /* KSTRING_H */
