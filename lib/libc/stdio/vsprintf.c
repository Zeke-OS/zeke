/* vsprintf( char *, const char *, va_list ap )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

int vsprintf( char * _PDCLIB_restrict s,
              const char * _PDCLIB_restrict format,
              va_list arg )
{
    return vsnprintf( s, SIZE_MAX, format, arg ); /* TODO: Replace with a non-checking call */
}
