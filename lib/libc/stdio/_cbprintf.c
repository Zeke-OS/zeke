/* _cbprintf( void *, size_t (*)( void*, const char *, size_t ), const char *, ... )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

int _cbprintf(
    void * p,
    size_t (*cb)( void*, const char*, size_t ),
    const char * _PDCLIB_restrict format,
    ...)
{
    int rc;
    va_list ap;
    va_start( ap, format );
    rc = _vcbprintf( p, cb, format, ap );
    va_end( ap );
    return rc;
}
