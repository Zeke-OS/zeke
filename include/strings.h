/**
 * @file    strings.h
 * @author  Olli Vanhoja
 * @brief
 * @section LICENSE
 * Copyright (c) 2002 Mike Barcroft <mike@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/** @addtogroup LIBC
  * @{
  */

#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#ifndef _SIZE_T_DECLARED
typedef __size_t size_t;
#define _SIZE_T_DECLARED
#endif

__BEGIN_DECLS

/**
 * @addtogroup ffs ffs, ffsl ffsll fls flsl flsll
 * Find first or last bit set in a word.
 * The ffs(), ffsl() and ffsll() functions find the first (least significant)
 * bit set in value and return the index of that bit.
 *
 * The fls(), flsl() and flsll() functions find the last (most significant)
 * bit set in value and return the index of that bit.
 *
 * Bits are numbered starting at 1, the least significant bit. A return
 * value of zero from any of these functions means that the argument was
 * zero.
 *
 * @sa bitstring(3)
 *
 * @since   The ffs() function appeared in 4.3BSD. Its prototype existed
 *          previously in <string.h> before it was moved to <strings.h> for
 *          IEEE Std 1003.1-2001 (``POSIX.1'') compliance.
 * @{
 */

#if __XSI_VISIBLE
/**
 * Find first bit set.
 * @param mask is the mask.
 */
int ffs(int mask) __pure2;
#endif

#if __BSD_VISIBLE
/**
 *
 */
int ffsl(long) __pure2;

/**
 *
 */
int ffsll(long long) __pure2;

/**
 *
 */
int fls(int) __pure2;

/**
 *
 */
int flsl(long) __pure2;

/**
 *
 */
int flsll(long long) __pure2;
#endif

/**
 * @}
 */

/**
 * @addtogroup strcasecmp strcasecmp, strncasecmp
 * Compare two strings ignoring case.
 * The strcasecmp() and strncasecmp() functions compare the null-terminated
 * strings s1 and s2.
 *
 * @return The functions strcasecmp() and strncasecmp() return an integer
 * greater than, equal to, or less than 0, depending on whether s1 is
 * lexicographically greater than, equal to, or less than s2 after translation
 * of each corresponding character to lower-case. The strings themselves are
 * not modified. The comparison is done using unsigned characters, so that
 * `\200' is greater than `\0'.
 *
 * @sa  bcmp(3), memcmp(3), strcmp(3), strcoll(3), strxfrm(3), tolower(3),
 *      wcscasecmp(3)
 *
 * @since The strcasecmp() and strncasecmp() functions first appeared in 4.4BSD.
 *        Their prototypes existed previously in <string.h> before they were
 *        moved to <strings.h> for IEEE Std 1003.1-2001 (``POSIX.1'')
 *        compliance.
 * @{
 */

/**
 * Compare null terminated strings.
 * @param s1 is the first string.
 * @param s2 is the second string.
 * @return Value depending on lexicographical order of the strings.
 */
int strcasecmp(const char * s1, const char * s2) __pure;

/**
 * Compare null terminated strings, at most len characters.
 * @param s1    is the first string.
 * @param s2    is the second string.
 * @param len   is the limit of characters to be compared.
 * @return Value depending on lexicographical order of the strings.
 */
int strncasecmp(const char *, const char *, size_t len) __pure;

/**
 * @}
 */

__END_DECLS

#endif /* _STRINGS_H_ */

/**
 * @}
 */
