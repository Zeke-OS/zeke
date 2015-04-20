
static int compare( const void * left, const void * right )
{
    return *( (unsigned char *)left ) - *( (unsigned char *)right );
}

int main( void )
{
    TESTCASE( bsearch( "e", abcde, 4, 1, compare ) == NULL );
    TESTCASE( bsearch( "e", abcde, 5, 1, compare ) == &abcde[4] );
    TESTCASE( bsearch( "a", abcde + 1, 4, 1, compare ) == NULL );
    TESTCASE( bsearch( "0", abcde, 1, 1, compare ) == NULL );
    TESTCASE( bsearch( "a", abcde, 1, 1, compare ) == &abcde[0] );
    TESTCASE( bsearch( "a", abcde, 0, 1, compare ) == NULL );
    TESTCASE( bsearch( "e", abcde, 3, 2, compare ) == &abcde[4] );
    TESTCASE( bsearch( "b", abcde, 3, 2, compare ) == NULL );
    return TEST_RESULTS;
}
