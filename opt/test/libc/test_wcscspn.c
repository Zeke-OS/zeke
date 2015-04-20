
int main( void )
{
    TESTCASE( wcscspn( wabcde, L"x" ) == 5 );
    TESTCASE( wcscspn( wabcde, L"xyz" ) == 5 );
    TESTCASE( wcscspn( wabcde, L"zyx" ) == 5 );
    TESTCASE( wcscspn( wabcdx, L"x" ) == 4 );
    TESTCASE( wcscspn( wabcdx, L"xyz" ) == 4 );
    TESTCASE( wcscspn( wabcdx, L"zyx" ) == 4 );
    TESTCASE( wcscspn( wabcde, L"a" ) == 0 );
    TESTCASE( wcscspn( wabcde, L"abc" ) == 0 );
    TESTCASE( wcscspn( wabcde, L"cba" ) == 0 );
    return TEST_RESULTS;
}
