#define _PDCLIB_FILEID "_PDCLIB/print.c"
#define _PDCLIB_STRINGIO

#include <_PDCLIB_test.h>

static size_t testcb( void *p, const char *buf, size_t size )
{
    char **destbuf = p;
    memcpy(*destbuf, buf, size);
    *destbuf += size;
    return size;
}

static int testprintf( char * buffer, const char * format, ... )
{
    /* Members: base, flags, n, i, current, width, prec, ctx, cb, arg      */
    struct _PDCLIB_status_t status;
    status.base = 0;
    status.flags = 0;
    status.n = 100;
    status.i = 0;
    status.current = 0;
    status.width = 0;
    status.prec = 0;
    status.ctx = &buffer;
    status.write = testcb;
    va_start( status.arg, format );
    memset( buffer, '\0', 100 );
    if ( _PDCLIB_print( format, &status ) != strlen( format ) )
    {
        printf( "_PDCLIB_print() did not return end-of-specifier on '%s'.\n", format );
        ++TEST_RESULTS;
    }
    va_end( status.arg );
    return status.i;
}

int main( void )
{
    char target[100];
#include "printf_testcases.h"
    return TEST_RESULTS;
}
