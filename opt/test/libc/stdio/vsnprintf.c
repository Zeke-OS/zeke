#define _PDCLIB_FILEID "stdio/vsnprintf.c"
#define _PDCLIB_STRINGIO
#include <stdint.h>
#include <stddef.h>
#include <_PDCLIB_test.h>

static int testprintf( char * s, const char * format, ... )
{
    int i;
    va_list arg;
    va_start( arg, format );
    i = vsnprintf( s, 100, format, arg );
    va_end( arg );
    return i;
}

int main( void )
{
    char target[100];
#include "printf_testcases.h"
    return TEST_RESULTS;
}
