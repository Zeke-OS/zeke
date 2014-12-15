#define _PDCLIB_FILEID "stdio/sprintf.c"
#define _PDCLIB_STRINGIO
#include <stddef.h>

#include <_PDCLIB_test.h>

static char * bufptr;
static size_t testcb( void *p, const char *buf, size_t size )
{
    memcpy(bufptr, buf, size);
    bufptr += size;
    *bufptr = '\0';
    return size;
}

#define testprintf( s, ... ) _cbprintf( bufptr = s, testcb, __VA_ARGS__ )

int main( void )
{
    char target[100];
#include "printf_testcases.h"
    return TEST_RESULTS;
}
