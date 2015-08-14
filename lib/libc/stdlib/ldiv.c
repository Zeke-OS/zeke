/*
 * ldiv(long int, long int)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

ldiv_t ldiv( long int numer, long int denom )
{
    ldiv_t rc;

    rc.quot = numer / denom;
    rc.rem  = numer % denom;

    return rc;
}
