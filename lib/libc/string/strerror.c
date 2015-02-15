/*
 * strerror( int )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <string.h>
#include <sys/_PDCLIB_locale.h>

char * strerror(int errnum)
{
    if (errnum == 0 || errnum >= _PDCLIB_ERRNO_MAX) {
        return (char *)"Unknown error";
    } else {
        return _PDCLIB_threadlocale()->_ErrnoStr[errnum];
    }
}
