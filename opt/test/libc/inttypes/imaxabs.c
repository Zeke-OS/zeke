#include <inttypes.h>
#include <limits.h>

int main( void )
{
    TESTCASE( imaxabs( (intmax_t)0 ) == 0 );
    TESTCASE( imaxabs( INTMAX_MAX ) == INTMAX_MAX );
    TESTCASE( imaxabs( INTMAX_MIN + 1 ) == -( INTMAX_MIN + 1 ) );

    return TEST_RESULTS;
}

