/*
 * fgetc( FILE * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <sys/_PDCLIB_io.h>

int _PDCLIB_fgetc_unlocked(FILE * stream)
{
    char c;
    size_t n;

    if (_PDCLIB_prepread(stream) == EOF) {
        return EOF;
    }

    n = _PDCLIB_getchars(&c, 1, EOF, stream);

    return (n == 0) ? EOF : (unsigned char)c;
}

int fgetc(FILE * stream)
{
    int c;

    _PDCLIB_flockfile(stream);
    c = _PDCLIB_fgetc_unlocked(stream);
    _PDCLIB_funlockfile(stream);

    return c;
}
