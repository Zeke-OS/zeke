/*
 * div(int, int)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

div_t div(int numer, int denom)
{
    div_t rc;

    rc.quot = numer / denom;
    rc.rem  = numer % denom;

    return rc;
}
