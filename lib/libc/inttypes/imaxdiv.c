/* lldiv( long long int, long long int )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <inttypes.h>

imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom)
{
    imaxdiv_t rc;

    rc.quot = numer / denom;
    rc.rem  = numer % denom;

    return rc;
}
