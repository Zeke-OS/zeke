
#include <limits.h>

int main( void )
{
    TESTCASE( abs( 0 ) == 0 );
    TESTCASE( abs( INT_MAX ) == INT_MAX );
    TESTCASE( abs( INT_MIN + 1 ) == -( INT_MIN + 1 ) );
    return TEST_RESULTS;
}
