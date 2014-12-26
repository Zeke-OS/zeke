/* strtoimax( const char *, char * *, int )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <limits.h>
#include <inttypes.h>
#include <stddef.h>

intmax_t strtoimax(const char * _PDCLIB_restrict nptr,
        char ** _PDCLIB_restrict endptr, int base)
{
    intmax_t rc;
    char sign = '+';
    const char * p = _PDCLIB_strtox_prelim(nptr, &sign, &base);

    if (base < 2 || base > 36)
        return 0;

    if (sign == '+') {
        rc = (intmax_t)_PDCLIB_strtox_main(&p, (unsigned)base,
                (uintmax_t)INTMAX_MAX, (uintmax_t)(INTMAX_MAX / base),
                (int)(INTMAX_MAX % base), &sign);
    } else {
        rc = (intmax_t)_PDCLIB_strtox_main(&p, (unsigned)base,
                (uintmax_t)INTMAX_MIN, (uintmax_t)(INTMAX_MIN / -base),
                (int)( -(INTMAX_MIN % base)), &sign);
    }
    if (endptr != NULL)
        *endptr = ( p != NULL ) ? (char *) p : (char *) nptr;

    return ( sign == '+' ) ? rc : -rc;
}
