/* TODO Fix qsort test */

#include <string.h>
#include <limits.h>

static int compare( const void * left, const void * right )
{
    return *( (unsigned char *)left ) - *( (unsigned char *)right );
}

int main( void )
{
    char presort[] = { "shreicnyjqpvozxmbt" };
    char sorted1[] = { "bcehijmnopqrstvxyz" };
    char sorted2[] = { "bticjqnyozpvreshxm" };
    char s[19];
    strcpy( s, presort );
    qsort( s, 18, 1, compare );
    TESTCASE( strcmp( s, sorted1 ) == 0 );
    strcpy( s, presort );
    qsort( s, 9, 2, compare );
    TESTCASE( strcmp( s, sorted2 ) == 0 );
    strcpy( s, presort );
    qsort( s, 1, 1, compare );
    TESTCASE( strcmp( s, presort ) == 0 );
#if defined(REGTEST) && (__BSD_VISIBLE || __APPLE__)
    puts( "qsort.c: Skipping test #4 for BSD as it goes into endless loop here." );
#else
    qsort( s, 100, 0, compare );
    TESTCASE( strcmp( s, presort ) == 0 );
#endif
    return TEST_RESULTS;
}
