/*
 * abs(int)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

int abs(int j)
{
    return (j >= 0) ? j : -j;
}
