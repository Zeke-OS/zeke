#define _PDCLIB_FILEID "stdio/vsscanf.c"
#define _PDCLIB_STRINGIO

#include <_PDCLIB_test.h>

static int testscanf( char const * stream, char const * format, ... )
{
    va_list ap;
    va_start( ap, format );
    int result = vsscanf( stream, format, ap );
    va_end( ap );
    return result;
}

int main( void )
{
    char source[100];
#include "scanf_testcases.h"
    return TEST_RESULTS;
}
