/* vprintf( const char *, va_list arg )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>

#include <sys/_PDCLIB_io.h>

int _PDCLIB_vprintf_unlocked( const char * _PDCLIB_restrict format,
                              _PDCLIB_va_list arg )
{
    return _PDCLIB_vfprintf_unlocked(stdout, format, arg);
}

int vprintf( const char * _PDCLIB_restrict format, _PDCLIB_va_list arg )
{
    return vfprintf(stdout, format, arg);
}
