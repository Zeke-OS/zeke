#define _PDCLIB_FILEID "stdio/fscanf.c"
#define _PDCLIB_FILEIO

#include <_PDCLIB_test.h>

#define testscanf( stream, format, ... ) fscanf( stream, format, __VA_ARGS__ )

int main( void )
{
    FILE * source;
    TESTCASE( ( source = tmpfile() ) != NULL );
#include "scanf_testcases.h"
    TESTCASE( fclose( source ) == 0 );
    return TEST_RESULTS;
}
