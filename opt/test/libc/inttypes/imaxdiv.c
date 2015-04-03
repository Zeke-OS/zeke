#include <inttypes.h>

int main( void )
{
    imaxdiv_t result;
    result = imaxdiv( (intmax_t)5, (intmax_t)2 );
    TESTCASE( result.quot == 2 && result.rem == 1 );
    result = imaxdiv( (intmax_t)-5, (intmax_t)2 );
    TESTCASE( result.quot == -2 && result.rem == -1 );
    result = imaxdiv( (intmax_t)5, (intmax_t)-2 );
    TESTCASE( result.quot == -2 && result.rem == 1 );
    TESTCASE( sizeof( result.quot ) == sizeof( intmax_t ) );
    TESTCASE( sizeof( result.rem )  == sizeof( intmax_t ) );

    return TEST_RESULTS;
}
