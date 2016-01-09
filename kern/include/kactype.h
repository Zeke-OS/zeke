/*
 * kactype.h 4.2 85/09/04
 *
 * This is the ctype from 2.11BSD. This old ctype header and ctype array is
 * included in the kernel to make fast verifications against ASCII C-strings.
 * 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 */

/**
 * @addtogroup libkern
 * @{
 */

/**
 * @addtogroup kactype
 * @{
 */

#pragma once
#ifndef KACTYPE_H
#define KACTYPE_H

#define _U  01
#define _L  02
#define _N  04
#define _S  010
#define _P  020
#define _C  040
#define _X  0100
#define _B  0200

extern char _kactype_[];

#define ka_isalpha(c)   ((_kactype_ + 1)[(int)(c)] & (_U | _L))
#define ka_isupper(c)   ((_kactype_ + 1)[(int)(c)] & _U)
#define ka_islower(c)   ((_kactype_ + 1)[(int)(c)] & _L)
#define ka_isdigit(c)   ((_kactype_ + 1)[(int)(c)] & _N)
#define ka_isxdigit(c)  ((_kactype_ + 1)[(int)(c)] & (_N | _X))
#define ka_isspace(c)   ((_kactype_ + 1)[(int)(c)] & _S)
#define ka_ispunct(c)   ((_kactype_ + 1)[(int)(c)] & _P)
#define ka_isalnum(c)   ((_kactype_ + 1)[(int)(c)] & (_U | _L | _N))
#define ka_isprint(c)   ((_kactype_ + 1)[(int)(c)] & (_P | _U | _L | _N | _B))
#define ka_isgraph(c)   ((_kactype_ + 1)[(int)(c)] & (_P | _U | _L | _N))
#define ka_iscntrl(c)   ((_kactype_ + 1)[(int)(c)] & _C)
#define ka_isascii(c)   ((unsigned)(c) <= 0177)
#define ka_toupper(c)   ((c) - 'a' + 'A')
#define ka_tolower(c)   ((c) - 'A' + 'a')
#define ka_toascii(c)   ((c) & 0177)

#endif /* KACTYPE_H */

/**
 * @}
 */

/**
 * @}
 */
