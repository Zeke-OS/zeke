#define _PDCLIB_FILEID "stdio/sscanf.c"
#define _PDCLIB_STRINGIO

#include <_PDCLIB_test.h>

#define testscanf( s, format, ... ) sscanf( s, format, __VA_ARGS__ )

int main( void )
{
    char source[100];
#include "scanf_testcases.h"
    return TEST_RESULTS;
}

