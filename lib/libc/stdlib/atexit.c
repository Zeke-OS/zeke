/*
 * atexit( void (*)( void ) )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>

extern void (*_PDCLIB_regstack[])(void);
extern size_t _PDCLIB_regptr;

int atexit( void (*func)(void))
{
    if (_PDCLIB_regptr == 0) {
        return -1;
    } else {
        _PDCLIB_regstack[--_PDCLIB_regptr] = func;
        return 0;
    }
}
