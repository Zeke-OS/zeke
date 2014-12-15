#include <stdint.h>
#include <stddef.h>
#define _PDCLIB_FILEID "stdio/fprintf.c"
#define _PDCLIB_FILEIO

#include <_PDCLIB_test.h>

#define testprintf( stream, ... ) fprintf( stream, __VA_ARGS__ )

int main(void)
{
    FILE * target;
    TESTCASE( ( target = tmpfile() ) != NULL );
#include "printf_testcases.h"
    TESTCASE( fclose( target ) == 0 );
    return TEST_RESULTS;
}
