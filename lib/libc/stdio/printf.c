/* printf( const char *, ... )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>

#include <sys/_PDCLIB_io.h>

int printf( const char * _PDCLIB_restrict format, ... )
{
    int rc;
    va_list ap;
    va_start( ap, format );
    rc = vfprintf( stdout, format, ap );
    va_end( ap );
    return rc;
}

int _PDCLIB_printf_unlocked( const char * _PDCLIB_restrict format, ... )
{
    int rc;
    va_list ap;
    va_start( ap, format );
    rc = _PDCLIB_vfprintf_unlocked( stdout, format, ap );
    va_end( ap );
    return rc;
}
