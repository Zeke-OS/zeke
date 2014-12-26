/* strtoumax( const char *, char * *, int )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <limits.h>
#include <inttypes.h>
#include <stddef.h>

uintmax_t strtoumax(const char * _PDCLIB_restrict nptr,
        char ** _PDCLIB_restrict endptr, int base)
{
    uintmax_t rc;
    char sign = '+';
    const char * p = _PDCLIB_strtox_prelim(nptr, &sign, &base);

    if (base < 2 || base > 36)
        return 0;

    rc = _PDCLIB_strtox_main(&p, (unsigned)base, (uintmax_t)UINTMAX_MAX,
            (uintmax_t)(UINTMAX_MAX / base), (int)(UINTMAX_MAX % base), &sign);
    if (endptr != NULL)
        *endptr = (p != NULL) ? (char *) p : (char *) nptr;

    return (sign == '+') ? rc : -rc;
}
