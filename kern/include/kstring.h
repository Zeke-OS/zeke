/**
 *******************************************************************************
 * @file    kstring.h
 * @author  Olli Vanhoja
 * @brief   String routines used within the kernel.
 * @section LICENSE
 * Copyright (c) 2021 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * @addtogroup kstring
 * @{
 */

#pragma once
#ifndef KSTRING_H
#define KSTRING_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/linker_set.h>

#define _TO_STR(x) #x
#define TO_STR(x) _TO_STR(x)

int memcmp(const void * s1, const void * s2, size_t n);
void * memcpy(void * restrict destination, const void * source, size_t num);
void * memmove(void * destination, const void * source, size_t num);
void * memset(void * ptr, int value, size_t num);

/**
 * @addtogroup strcmp
 * @{
 */

/**
 * Compares the C string str1 to the C string str2.
 * @param str1 String 1.
 * @param str2 String 2.
 * @return A zero value indicates that both strings are equal.
 */
int strcmp(const char * str1, const char * str2);

/**
 * Compares the C string str1 to the C string str2.
 * @param str1 String 1.
 * @param str2 String 2.
 * @param Maximum number of characters to compare.
 * @return A zero value indicates that both strings are equal.
 */
int strncmp(const char * str1, const char * str2, size_t n);

/**
 * @}
 */

/**
 * @addtogroup strcpy strcpy, strncpy, strlcpy, strscpy
 * @{
 */

/**
 * Copy string.
 * @param destination Pointer to the destination string.
 * @param source Pointer to the source string.
 */
char * strcpy(char * destination, const char * source);

/**
 * Copy characters from string.
 * Copies the first n characters of source to destination.
 * If the end of the source string (which is signaled by a null-character) is
 * found before n characters have been copied, destination is padded with
 * zeros until a total of num characters have been written to it.
 *
 * No null-character is implicitly appended at the end of destination if
 * source is longer than n. Thus, in this case, destination shall not
 * be considered a null terminated C string (reading it as such would overflow).
 * @param dst pointer to the destination array.
 * @param src string to be copied from.
 * @param n is the maximum number of characters to be copied from src.
 * @return dst.
 */
char * strncpy(char * dst, const char * src, size_t n);

/**
 * Copy src to string dst of size siz.
 * At most siz-1 characters will be copied.
 * Always NUL terminates (unless siz == 0).
 * @param dst is the destination.
 * @param src is the source C-string.
 * @param siz is the maximum size copied.
 * @return Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char * dst, const char * src, size_t siz);

/**
 * Copy src to string dst of size siz.
 * If truncation occurs the dst string is NUL terminated and -E2BIG is returned.
 * @return Returns the number of characters copied if src was fully copied until
 *         a NUL character;
 *         If the destination is truncated then -E2BIG is returned.
 */
ssize_t strscpy(char * dst, const char * src, size_t siz);

/**
 * @}
 */

const char * strstr(const char * str1, const char * str2);

/**
 * Get string length.
 * @param str null terminated string.
 * @param max Maximum length counted.
 */
size_t strlenn(const char * str, size_t max);

/**
 * Concatenate strings.
 * Appends a copy of the src to the dst string.
 * @param dst is the destination array.
 * @param ndst maximum length of dst.
 * @param src is the source string array.
 * @param nsrc maximum number of characters to bo copied from src.
 */
char * strnncat(char * dst, size_t ndst, const char * src, size_t nsrc);

/** @addtogroup strsep strsep
 * Get next token.
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 * @{
 */

char * strsep(char ** stringp, const char * delim);

/**
 * @}
 */

/**
 * Locate last occurence of character in string.
 * @return Returns a pointer to the last occurrence of character in str;
 *         If the character is not found then NULL.
 */
char * kstrrchr(const char * p, char ch);

/**
 * Locate first occurence of character in string.
 */
char * kstrchr(const char * p, char ch);

/**
 * Validate null terminated string.
 * Return (1) if the buffer pointed to by kernel pointer 'buffer' and
 * of length 'bufferlen' contains a valid NUL-terminated string.
 * @param buffer
 */
int strvalid(const char * buf, size_t len);

int atoi(const char * str);

/**
 * Convert uint32_t integer to a decimal string.
 * @param str   is an array in memory where the resulting
 *              null-terminated string is written to.
 * @param value is the value to be converted to a string.
 * @return      Number of characters inserted.
 */
int uitoa32(char * str, uint32_t value);

/**
 * Convert uint64_t integer to a decimal string.
 * @param str   is an array in memory where the resulting
 *              null-terminated string is written to.
 * @param value is the value to be converted to a string.
 * @return      Number of characters inserted.
 */
int uitoa64(char * str, uint64_t value);

/**
 * Compute the number of characters needed to print an integer.
 * @return Returns the number of characters needed to print value.
 */
int ui64_chcnt(uint64_t value);

/**
 * Covert an unsigned integer to a hex string.
 * @{
 */

/**
 * Convert uint32_t integer to a hex string.
 * @param str       is an array in memory where resulting null-terminated
 *                  string is written to.
 * @param value     is the value to be converted to a string.
 * @return number of characters inserted.
 */
int uitoah32(char * str, uint32_t value);

/**
 * Convert uint64_t integer to a hex string.
 * @param str       is an array in memory where resulting null-terminated
 *                  string is written to.
 * @param value     is the value to be converted to a string.
 * @return number of characters inserted.
 */
int uitoah64(char * str, uint64_t value);

/**
 * @}
 */

/**
 * Convert an integer to a digit string of a selected base.
 * @{
 */

/**
 * Convert uint32_t integer to a digit string of a selected base.
 * @param str       is an array in memory where resulting null-terminated
 *                  string is written to.
 * @param value     is the value to be converted to a string.
 * @param base      is the base used.
 * @return number of characters inserted.
 */
int uitoa32base(char * str, uint32_t value, uint32_t base);

/**
 * Convert uint63_t integer to a digit string of a selected base.
 * @param str       is an array in memory where resulting null-terminated
 *                  string is written to.
 * @param value     is the value to be converted to a string.
 * @param base      is the base used.
 * @return number of characters inserted.
 */
int uitoa64base(char * str, uint64_t value, uint64_t base);

/**
 * @}
 */

/**
 * Duplicate a string.
 * @param src is a pointer to the source string.
 * @param max is the maximum length of the src string.
 * @return A kmalloc'd pointer to the copied string.
 */
char * kstrdup(const char * src, size_t max);

/**
 * @addtogroup ksprintf
 * Formated string composer.
 * @{
 */

/**
 * ksprintf formatter flags.
 * @{
 */

#define KSPRINTF_FMTFLAG_i  0x0001 /*!< Default width supported. */
#define KSPRINTF_FMTFLAG_p  0x0002 /*!< Pointers supported. */
#define KSPRINTF_FMTFLAG_hh 0x0004 /*!< hh sub-specifier supported. */
#define KSPRINTF_FMTFLAG_h  0x0008 /*!< h sub-specifier supported. */
#define KSPRINTF_FMTFLAG_l  0x0010 /*!< l sub-specifier supported. */
#define KSPRINTF_FMTFLAG_ll 0x0020 /*!< ll sub-specifier supported. */
#define KSPRINTF_FMTFLAG_j  0x0040 /*!< j sub-specifier supported. */
#define KSPRINTF_FMTFLAG_z  0x0080 /*!< z sub-specifier supported. */

/**
 * @}
 */

#define KSPRINTF_FMTFUN_ARGS \
    char * str, void * value_p, size_t value_size, int maxlen

struct ksprintf_formatter {
    uint16_t flags;     /*!< Formatter compatibility flags. */
    char specifier;     /*!< Specifier. */
    char alt_specifier; /*!< Alternative specifier. */
    char p_specifier;   /*!< Pointer type sub-specifier. Must be upper case. */
    int (*func)(KSPRINTF_FMTFUN_ARGS);
};

SET_DECLARE(ksprintf_formatters, struct ksprintf_formatter);


#define KSPRINTF_FORMATTER(_fmt_struct_) \
    DATA_SET(ksprintf_formatters, _fmt_struct_)

/**
 * Composes a string by using printf style format string and additional
 * arguments.
 *
 * This function supports following format specifiers:
 *      %d or i Signed decimal integer
 *      %u      Unsigned decimal integer
 *      %x      Unsigned hexadecimal integer
 *      %c      Character
 *      %s      String of characters
 *      %s      Pointer address
 *      %%      Replaced with a single %
 *
 * @param str Pointer to a buffer where the resulting string is stored.
 * @param maxlen Maximum length of str. Replacements may cause writing of more
 *               than maxlen characters!
 * @param format String that contains a format string.
 * @param ... Depending on the format string, a sequence of additional
 *            arguments, each containing a value to be used to replace
 *            a format specifier in the format string.
 */
int ksprintf(char * str, size_t maxlen, const char * format, ...)
    __attribute__ ((format (printf, 3, 4)));

/**
 * @}
 */

char * kstrtok(char * s, const char * delim, char ** lasts);

#endif /* KSTRING_H */

/**
 * @}
 */
