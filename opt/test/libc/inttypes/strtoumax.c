#include <limits.h>
#include <inttypes.h>
#include <errno.h>

int main( void )
{
    char * endptr;
    /* this, to base 36, overflows even a 256 bit integer */
    char overflow[] = "-ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ_";
    /* tricky border case */
    char tricky[] = "+0xz";
    errno = 0;
    /* basic functionality */
    TESTCASE( strtoumax( "123", NULL, 10 ) == 123 );
    /* proper detecting of default base 10 */
    TESTCASE( strtoumax( "456", NULL, 0 ) == 456 );
    /* proper functioning to smaller base */
    TESTCASE( strtoumax( "14", NULL, 8 ) == 12 );
    /* proper autodetecting of octal */
    TESTCASE( strtoumax( "016", NULL, 0 ) == 14 );
    /* proper autodetecting of hexadecimal, lowercase 'x' */
    TESTCASE( strtoumax( "0xFF", NULL, 0 ) == 255 );
    /* proper autodetecting of hexadecimal, uppercase 'X' */
    TESTCASE( strtoumax( "0Xa1", NULL, 0 ) == 161 );
    /* proper handling of border case: 0x followed by non-hexdigit */
    TESTCASE( strtoumax( tricky, &endptr, 0 ) == 0 );
    TESTCASE( endptr == tricky + 2 );
    /* proper handling of border case: 0 followed by non-octdigit */
    TESTCASE( strtoumax( tricky, &endptr, 8 ) == 0 );
    TESTCASE( endptr == tricky + 2 );
    /* errno should still be 0 */
    TESTCASE( errno == 0 );
    /* overflowing subject sequence must still return proper endptr */
    TESTCASE( strtoumax( overflow, &endptr, 36 ) == UINTMAX_MAX );
    TESTCASE( errno == ERANGE );
    TESTCASE( ( endptr - overflow ) == 53 );
    /* same for positive */
    errno = 0;
    TESTCASE( strtoumax( overflow + 1, &endptr, 36 ) == UINTMAX_MAX );
    TESTCASE( errno == ERANGE );
    TESTCASE( ( endptr - overflow ) == 53 );
    /* testing skipping of leading whitespace */
    TESTCASE( strtoumax( " \n\v\t\f789", NULL, 0 ) == 789 );
    /* testing conversion failure */
    TESTCASE( strtoumax( overflow, &endptr, 10 ) == 0 );
    TESTCASE( endptr == overflow );
    endptr = NULL;
    TESTCASE( strtoumax( overflow, &endptr, 0 ) == 0 );
    TESTCASE( endptr == overflow );
    errno = 0;
/* uintmax_t -> long long -> 64 bit */
#if UINTMAX_MAX >> 63 == 1
    /* testing "odd" overflow, i.e. base is not power of two */
    TESTCASE( strtoumax( "18446744073709551615", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == 0 );
    TESTCASE( strtoumax( "18446744073709551616", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == ERANGE );
    /* testing "even" overflow, i.e. base is power of two */
    errno = 0;
    TESTCASE( strtoumax( "0xFFFFFFFFFFFFFFFF", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == 0 );
    TESTCASE( strtoumax( "0x10000000000000000", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == ERANGE );
/* uintmax_t -> long long -> 128 bit */
#elif UINTMAX_MAX >> 127 == 1
    /* testing "odd" overflow, i.e. base is not power of two */
    TESTCASE( strtoumax( "340282366920938463463374607431768211455", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == 0 );
    TESTCASE( strtoumax( "340282366920938463463374607431768211456", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == ERANGE );
    /* testing "even" everflow, i.e. base is power of two */
    errno = 0;
    TESTCASE( strtoumax( "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == 0 );
    TESTCASE( strtoumax( "0x100000000000000000000000000000000", NULL, 0 ) == UINTMAX_MAX );
    TESTCASE( errno == ERANGE );
#else
#error Unsupported width of 'uintmax_t' (neither 64 nor 128 bit).
#endif
    return TEST_RESULTS;
}
