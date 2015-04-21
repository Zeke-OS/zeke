/*
 * ftell( FILE * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <sys/_PDCLIB_io.h>

long int _PDCLIB_ftell_unlocked(FILE * stream)
{
    uint_fast64_t off64 = _PDCLIB_ftell64_unlocked(stream);

    if (off64 > LONG_MAX)
    {
        /* integer overflow */
        errno = ERANGE;
        return -1;
    }

    return off64;
}

long int ftell(FILE * stream)
{
    long int off;

    _PDCLIB_flockfile(stream);
    off = _PDCLIB_ftell_unlocked(stream);
    _PDCLIB_funlockfile(stream);

    return off;
}
