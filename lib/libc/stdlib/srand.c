/*
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

void srand(unsigned int seed)
{
    _PDCLIB_seed = seed;
}
