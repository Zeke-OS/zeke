/*
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

unsigned long int strtoul(const char * s, char ** endptr, int base)
{
    unsigned long int rc;
    char sign = '+';
    const char * p = _PDCLIB_strtox_prelim( s, &sign, &base );

    if ( base < 2 || base > 36 )
        return 0;

    rc = (unsigned long int)_PDCLIB_strtox_main(&p, (unsigned)base,
                                                (uintmax_t)ULONG_MAX,
                                                (uintmax_t)(ULONG_MAX / base),
                                                (int)(ULONG_MAX % base),
                                                &sign);
    if (endptr != NULL)
        *endptr = (p != NULL) ? (char *)p : (char *)s;

    return (sign == '+') ? rc : -rc;
}
