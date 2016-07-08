/*
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

int rand(void)
{
    _PDCLIB_seed = _PDCLIB_seed * 1103515245 + 12345;
    return (int)( _PDCLIB_seed / 65536) % 32768;
}
