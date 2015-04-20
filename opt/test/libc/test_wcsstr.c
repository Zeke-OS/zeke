
int main( void )
{
    wchar_t s[] = L"abcabcabcdabcde";
    TESTCASE( wcsstr( s, L"x" ) == NULL );
    TESTCASE( wcsstr( s, L"xyz" ) == NULL );
    TESTCASE( wcsstr( s, L"a" ) == &s[0] );
    TESTCASE( wcsstr( s, L"abc" ) == &s[0] );
    TESTCASE( wcsstr( s, L"abcd" ) == &s[6] );
    TESTCASE( wcsstr( s, L"abcde" ) == &s[10] );
    return TEST_RESULTS;
}
