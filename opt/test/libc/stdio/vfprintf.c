#define _PDCLIB_FILEID "stdio/vfprintf.c"
#define _PDCLIB_FILEIO
#include <stddef.h>
#include <_PDCLIB_test.h>

static int testprintf( FILE * stream, const char * format, ... )
{
    int i;
    va_list arg;
    va_start( arg, format );
    i = vfprintf( stream, format, arg );
    va_end( arg );
    return i;
}

int main( void )
{
    FILE * target;
    TESTCASE( ( target = tmpfile() ) != NULL );
#include "printf_testcases.h"
    TESTCASE( fclose( target ) == 0 );
    return TEST_RESULTS;
}
