
#include <_PDCLIB_test.h>
#include <string.h>

int main( void )
{
    FILE * fh;
    fpos_t pos1, pos2;
    TESTCASE( ( fh = tmpfile() ) != NULL );
    TESTCASE( fgetpos( fh, &pos1 ) == 0 );
    TESTCASE( fwrite( teststring, 1, strlen( teststring ), fh ) == strlen( teststring ) );
    TESTCASE( (size_t)ftell( fh ) == strlen( teststring ) );
    TESTCASE( fgetpos( fh, &pos2 ) == 0 );
    TESTCASE( fsetpos( fh, &pos1 ) == 0 );
    TESTCASE( ftell( fh ) == 0 );
    TESTCASE( fsetpos( fh, &pos2 ) == 0 );
    TESTCASE( (size_t)ftell( fh ) == strlen( teststring ) );
    TESTCASE( fclose( fh ) == 0 );
    return TEST_RESULTS;
}
