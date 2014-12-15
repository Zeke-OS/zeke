#define _PDCLIB_FILEID "stdio/snprintf.c"
#define _PDCLIB_STRINGIO
#include <stdint.h>
#include <stddef.h>

#include <_PDCLIB_test.h>

#define testprintf( s, ... ) snprintf( s, 100, __VA_ARGS__ )

int main( void )
{
    char target[100];
#include "printf_testcases.h"
    TESTCASE( snprintf( NULL, 0, "foo" ) == 3 );
    TESTCASE( snprintf( NULL, 0, "%d", 100 ) == 3 );
    return TEST_RESULTS;
}
