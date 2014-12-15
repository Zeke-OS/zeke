/* vfprintf( FILE *, const char *, va_list )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#include <sys/_PDCLIB_io.h>

static size_t filecb(void *p, const char *buf, size_t size)
{
    return _PDCLIB_fwrite_unlocked( buf, 1, size, (FILE*) p );
}

int _PDCLIB_vfprintf_unlocked( FILE * _PDCLIB_restrict stream,
                       const char * _PDCLIB_restrict format,
                       va_list arg )
{
    return _vcbprintf(stream, filecb, format, arg);
}

int vfprintf( FILE * _PDCLIB_restrict stream,
              const char * _PDCLIB_restrict format,
              va_list arg )
{
    _PDCLIB_flockfile( stream );
    int r = _PDCLIB_vfprintf_unlocked( stream, format, arg );
    _PDCLIB_funlockfile( stream );

    return r;
}
