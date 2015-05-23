/*
 * _PDCLIB_atomax(const char *)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <sys/_PDCLIB_int.h>
#include <string.h>
#include <ctype.h>

_PDCLIB_intmax_t _PDCLIB_atomax(const char * s)
{
    _PDCLIB_intmax_t rc = 0;
    char sign = '+';
    const char * x;

    /* TODO: In other than "C" locale, additional patterns may be defined  */
    while (isspace(*s))
        ++s;

    if (*s == '+')
        ++s;
    else if (*s == '-')
        sign = *(s++);
    while ((x = memchr(_PDCLIB_digits, tolower(*(s++)), 10)) != NULL) {
        rc = rc * 10 + (x - _PDCLIB_digits);
    }
    return (sign == '+') ? rc : -rc;
}
