/*
 *  lldiv(long long int, long long int)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

lldiv_t lldiv(long long int numer, long long int denom)
{
    lldiv_t rc;

    rc.quot = numer / denom;
    rc.rem  = numer % denom;

    return rc;
}
