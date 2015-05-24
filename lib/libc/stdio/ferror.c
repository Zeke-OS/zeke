/*
 * ferror( FILE * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <sys/_PDCLIB_io.h>

int _PDCLIB_ferror_unlocked(FILE * stream)
{
    return stream->status & _PDCLIB_ERRORFLAG;
}

int ferror(FILE * stream)
{
    int error;

    _PDCLIB_flockfile(stream);
    error = _PDCLIB_ferror_unlocked(stream);
    _PDCLIB_funlockfile(stream);

    return error;
}
