#define _PDCLIB_FILEID "stdio/_vcbprintf.c"
#define _PDCLIB_STRINGIO
#include <stdint.h>
#include <stddef.h>
#include <_PDCLIB_test.h>

static size_t testcb( void *p, const char *buf, size_t size )
{
    char **destbuf = p;
    memcpy(*destbuf, buf, size);
    *destbuf += size;
    return size;
}

static int testprintf( char * s, const char * format, ... )
{
    int i;
    va_list arg;
    va_start( arg, format );
    i = _vcbprintf( &s, testcb, format, arg );
    *s = 0;
    va_end( arg );
    return i;
}

int main( void )
{
    char target[100];
#include "printf_testcases.h"
    return TEST_RESULTS;
}
