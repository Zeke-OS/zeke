#define _PDCLIB_FILEID "stdio/printf.c"
#define _PDCLIB_FILEIO
#include <stdint.h>
#include <stddef.h>

#include <_PDCLIB_test.h>

#define testprintf( stream, ... ) printf( __VA_ARGS__ )

int main( void )
{
    FILE * target;
    TESTCASE( ( target = freopen( testfile, "wb+", stdout ) ) != NULL );
#include "printf_testcases.h"
    TESTCASE( fclose( target ) == 0 );
    TESTCASE( remove( testfile ) == 0 );
    return TEST_RESULTS;
}
