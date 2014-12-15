/* scanf( const char *, ... )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>

#include <sys/_PDCLIB_io.h>

int _PDCLIB_scanf_unlocked( const char * _PDCLIB_restrict format, ... )
{
    va_list ap;
    va_start( ap, format );
    return _PDCLIB_vfscanf_unlocked( stdin, format, ap );
}

int scanf( const char * _PDCLIB_restrict format, ... )
{
    va_list ap;
    va_start( ap, format );
    return vfscanf( stdin, format, ap );
}
