/*
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

unsigned long long int strtoull(const char * s, char ** endptr, int base)
{
    unsigned long long int rc;
    char sign = '+';
    const char * p = _PDCLIB_strtox_prelim(s, &sign, &base);

    if (base < 2 || base > 36)
        return 0;

    rc = _PDCLIB_strtox_main(&p, (unsigned)base, (uintmax_t)ULLONG_MAX,
                             (uintmax_t)(ULLONG_MAX / base),
                             (int)(ULLONG_MAX % base), &sign);
    if (endptr != NULL)
        *endptr = (p != NULL) ? (char *)p : (char *)s;

    return (sign == '+') ? rc : -rc;
}
