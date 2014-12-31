#define _PDCLIB_FILEID "_PDCLIB/scan.c"
#define _PDCLIB_STRINGIO

#include <_PDCLIB_test.h>

static int testscanf( char const * s, char const * format, ... )
{
    struct _PDCLIB_status_t status;
    status.n = 0;
    status.i = 0;
    status.s = (char *)s;
    status.stream = NULL;
    va_start( status.arg, format );
    if ( *(_PDCLIB_scan( format, &status )) != '\0' )
    {
        printf( "_PDCLIB_scan() did not return end-of-specifier on '%s'.\n", format );
        ++TEST_RESULTS;
    }
    va_end( status.arg );
    return status.n;
}

#define TEST_CONVERSION_ONLY

int main( void )
{
    char source[100];
#include "scanf_testcases.h"
    return TEST_RESULTS;
}
