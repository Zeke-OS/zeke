#include <stdio.h>

int main( void )
{
    FILE * fh;
    TESTCASE( ( fh = tmpfile() ) != NULL );
    /* Flags should be clear */
    TESTCASE( ! ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    /* Reading from input stream - should provoke error */
    /* FIXME: Apparently glibc disagrees on this assumption. How to provoke error on glibc? */
    TESTCASE( fgetc( fh ) == EOF );
    TESTCASE( ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    /* clearerr() should clear flags */
    clearerr( fh );
    TESTCASE( ! ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    /* Reading from empty stream - should provoke EOF */
    rewind( fh );
    TESTCASE( fgetc( fh ) == EOF );
    TESTCASE( ! ferror( fh ) );
    TESTCASE( feof( fh ) );
    /* clearerr() should clear flags */
    clearerr( fh );
    TESTCASE( ! ferror( fh ) );
    TESTCASE( ! feof( fh ) );
    TESTCASE( fclose( fh ) == 0 );
    return TEST_RESULTS;
}

