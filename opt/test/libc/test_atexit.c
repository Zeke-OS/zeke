
#include <assert.h>

static int flags[ 32 ];

static void counthandler( void )
{
    static int count = 0;
    flags[ count ] = count;
    ++count;
}

static void checkhandler( void )
{
    for ( int i = 0; i < 31; ++i )
    {
        assert( flags[ i ] == i );
    }
}

int main( void )
{
    TESTCASE( atexit( &checkhandler ) == 0 );
    for ( int i = 0; i < 31; ++i )
    {
        TESTCASE( atexit( &counthandler ) == 0 );
    }
    return TEST_RESULTS;
}
