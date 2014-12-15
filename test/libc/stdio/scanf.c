#define _PDCLIB_FILEID "stdio/scanf.c"
#define _PDCLIB_FILEIO

#include <_PDCLIB_test.h>

#define testscanf( stream, format, ... ) scanf( format, __VA_ARGS__ )

int main( void )
{
    FILE * source;
    TESTCASE( ( source = freopen( testfile, "wb+", stdin ) ) != NULL );
#include "scanf_testcases.h"
    TESTCASE( fclose( source ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
    return TEST_RESULTS;
}
