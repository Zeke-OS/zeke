/* $Id$ */
/*
 * ungetc( int, FILE * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdio.h>
#include <sys/_PDCLIB_io.h>

int _PDCLIB_ungetc_unlocked(int c, FILE * stream)
{
    if (c == EOF || stream->ungetidx == _PDCLIB_UNGETCBUFSIZE)
    {
        return -1;
    }

    return stream->ungetbuf[stream->ungetidx++] = (unsigned char)c;
}

int ungetc(int c, FILE * stream)
{
    int r;

    _PDCLIB_flockfile(stream);
    r = _PDCLIB_ungetc_unlocked(c, stream);

    _PDCLIB_funlockfile(stream);

    return r;
}
