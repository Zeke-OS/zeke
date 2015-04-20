
int main( void )
{
    TESTCASE( wcspbrk( wabcde, L"x" ) == NULL );
    TESTCASE( wcspbrk( wabcde, L"xyz" ) == NULL );
    TESTCASE( wcspbrk( wabcdx, L"x" ) == &wabcdx[4] );
    TESTCASE( wcspbrk( wabcdx, L"xyz" ) == &wabcdx[4] );
    TESTCASE( wcspbrk( wabcdx, L"zyx" ) == &wabcdx[4] );
    TESTCASE( wcspbrk( wabcde, L"a" ) == &wabcde[0] );
    TESTCASE( wcspbrk( wabcde, L"abc" ) == &wabcde[0] );
    TESTCASE( wcspbrk( wabcde, L"cba" ) == &wabcde[0] );
    return TEST_RESULTS;
}
