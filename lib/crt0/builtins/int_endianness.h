/* ===-- int_endianness.h - configuration header for compiler-rt ------------===
 *
 *		       The LLVM Compiler Infrastructure
 *
 * This file is dual licensed under the MIT and the University of Illinois Open
 * Source Licenses. See LICENSE.TXT for details.
 *
 * ===----------------------------------------------------------------------===
 *
 * This file is a configuration header for compiler-rt.
 * This file is not part of the interface of this library.
 *
 * ===----------------------------------------------------------------------===
 */

#ifndef INT_ENDIANNESS_H
#define INT_ENDIANNESS_H

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) ||   \
    defined(__minix) || defined(__ZEKE__)
#include <sys/endian.h>

#if _BYTE_ORDER == _BIG_ENDIAN
#define _YUGA_LITTLE_ENDIAN 0
#define _YUGA_BIG_ENDIAN    1
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define _YUGA_LITTLE_ENDIAN 1
#define _YUGA_BIG_ENDIAN    0
#endif /* _BYTE_ORDER */

#endif /* *BSD */

#if defined(__OpenBSD__) || defined(__Bitrig__)
#include <machine/endian.h>

#if _BYTE_ORDER == _BIG_ENDIAN
#define _YUGA_LITTLE_ENDIAN 0
#define _YUGA_BIG_ENDIAN    1
#elif _BYTE_ORDER == _LITTLE_ENDIAN
#define _YUGA_LITTLE_ENDIAN 1
#define _YUGA_BIG_ENDIAN    0
#endif /* _BYTE_ORDER */

#endif /* OpenBSD and Bitrig. */

#if !defined(_YUGA_LITTLE_ENDIAN) || !defined(_YUGA_BIG_ENDIAN)
#error Unable to determine endian
#endif /* Check we found an endianness correctly. */

#endif /* INT_ENDIANNESS_H */
