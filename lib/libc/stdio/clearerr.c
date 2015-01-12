/* clearerr( FILE * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>

#include <sys/_PDCLIB_io.h>

void _PDCLIB_clearerr_unlocked(FILE * stream)
{
    stream->status &= ~(_PDCLIB_ERRORFLAG | _PDCLIB_EOFFLAG);
}

void clearerr(FILE * stream)
{
    _PDCLIB_flockfile(stream);
    _PDCLIB_clearerr_unlocked(stream);
    _PDCLIB_funlockfile(stream);
}
