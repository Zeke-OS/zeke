/*
 * feof( FILE * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <sys/_PDCLIB_io.h>

int _PDCLIB_feof_unlocked(FILE * stream)
{
    return stream->status & _PDCLIB_EOFFLAG;
}

int feof(FILE * stream)
{
    int eof;

    _PDCLIB_flockfile(stream);
    eof = _PDCLIB_feof_unlocked(stream);
    _PDCLIB_funlockfile(stream);

    return eof;
}
