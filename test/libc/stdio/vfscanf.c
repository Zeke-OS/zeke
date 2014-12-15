#define _PDCLIB_FILEID "stdio/vfscanf.c"
#define _PDCLIB_FILEIO

#include <_PDCLIB_test.h>

static int testscanf( FILE * stream, char const * format, ... )
{
    va_list ap;
    va_start( ap, format );
    int result = vfscanf( stream, format, ap );
    va_end( ap );
    return result;
}

int main( void )
{
    FILE * source;
    TESTCASE( ( source = tmpfile() ) != NULL );
#include "scanf_testcases.h"
    TESTCASE( fclose( source ) == 0 );
    return TEST_RESULTS;
}
