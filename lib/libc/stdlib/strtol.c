/*
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

long int strtol(const char * s, char ** endptr, int base)
{
    long int rc;
    char sign = '+';
    const char * p = _PDCLIB_strtox_prelim(s, &sign, &base);

    if (base < 2 || base > 36)
        return 0;

    if (sign == '+') {
        rc = (long int)_PDCLIB_strtox_main(&p, (unsigned)base,
                                           (uintmax_t)LONG_MAX,
                                           (uintmax_t)(LONG_MAX / base),
                                           (int)(LONG_MAX % base),
                                           &sign);
    } else {
        rc = (long int)_PDCLIB_strtox_main(&p, (unsigned)base,
                                           (uintmax_t)LONG_MIN,
                                           (uintmax_t)(LONG_MIN / -base),
                                           (int)(-(LONG_MIN % base)),
                                           &sign);
    }
    if (endptr != NULL)
        *endptr = (p != NULL) ? (char *)p : (char *)s;

    return (sign == '+') ? rc : -rc;
}
