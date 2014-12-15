#define _PDCLIB_FILEID "stdio/sprintf.c"
#define _PDCLIB_STRINGIO
#include <stddef.h>

#include <_PDCLIB_test.h>

#define testprintf( s, ... ) sprintf( s, __VA_ARGS__ )

int main( void )
{
    char target[100];
#include "printf_testcases.h"
    return TEST_RESULTS;
}
