/*
 * fgets( char *, int, FILE * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <sys/_PDCLIB_io.h>

char * _PDCLIB_fgets_unlocked(char * _PDCLIB_restrict s, int size,
                              FILE * _PDCLIB_restrict stream)
{
    char * dest;

    if (size == 0) {
        return NULL;
    }
    if (size == 1) {
        *s = '\0';
        return s;
    }
    if (_PDCLIB_prepread(stream) == EOF) {
        return NULL;
    }

    dest = s;
    dest += _PDCLIB_getchars(dest, size - 1, '\n', stream);
    *dest = '\0';

    return (dest == s) ? NULL : s;
}

char * fgets(char * _PDCLIB_restrict s, int size,
             FILE * _PDCLIB_restrict stream)
{
    char * r;

    _PDCLIB_flockfile(stream);
    r = _PDCLIB_fgets_unlocked(s, size, stream);
    _PDCLIB_funlockfile(stream);

    return r;
}
