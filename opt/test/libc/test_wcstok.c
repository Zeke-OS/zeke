
int main( void )
{
    // MinGW at least has a very nonconforming (different signature!) variety
    // of wcstok
#ifndef REGTEST
    wchar_t s[] = L"_a_bc__d_";
    wchar_t* state  = NULL;
    wchar_t* tokres;

    TESTCASE( ( tokres = wcstok( s, L"_", &state ) ) == &s[1] );
    TESTCASE( s[1] == L'a' );
    TESTCASE( s[2] == L'\0' );
    TESTCASE( ( tokres = wcstok( NULL, L"_", &state ) ) == &s[3] );
    TESTCASE( s[3] == L'b' );
    TESTCASE( s[4] == L'c' );
    TESTCASE( s[5] == L'\0' );
    TESTCASE( ( tokres = wcstok( NULL, L"_", &state ) ) == &s[7] );
    TESTCASE( s[6] == L'_' );
    TESTCASE( s[7] == L'd' );
    TESTCASE( s[8] == L'\0' );
    TESTCASE( ( tokres = wcstok( NULL, L"_", &state ) ) == NULL );
    wcscpy( s, L"ab_cd" );
    TESTCASE( ( tokres = wcstok( s, L"_", &state ) ) == &s[0] );
    TESTCASE( s[0] == L'a' );
    TESTCASE( s[1] == L'b' );
    TESTCASE( s[2] == L'\0' );
    TESTCASE( ( tokres = wcstok( NULL, L"_", &state ) ) == &s[3] );
    TESTCASE( s[3] == L'c' );
    TESTCASE( s[4] == L'd' );
    TESTCASE( s[5] == L'\0' );
    TESTCASE( ( tokres = wcstok( NULL, L"_", &state ) ) == NULL );
#endif
    return TEST_RESULTS;
}
