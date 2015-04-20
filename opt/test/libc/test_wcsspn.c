
int main( void )
{
    TESTCASE( wcsspn( wabcde, L"abc" ) == 3 );
    TESTCASE( wcsspn( wabcde, L"b" ) == 0 );
    TESTCASE( wcsspn( wabcde, wabcde ) == 5 );
    return TEST_RESULTS;
}
