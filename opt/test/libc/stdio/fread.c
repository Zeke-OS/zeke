
int main( void )
{
    FILE * fh;
    char const * message = "Testing fwrite()...\n";
    char buffer[21];
    buffer[20] = 'x';
    TESTCASE( ( fh = tmpfile() ) != NULL );
    /* fwrite() / readback */
    TESTCASE( fwrite( message, 1, 20, fh ) == 20 );
    rewind( fh );
    TESTCASE( fread( buffer, 1, 20, fh ) == 20 );
    TESTCASE( memcmp( buffer, message, 20 ) == 0 );
    TESTCASE( buffer[20] == 'x' );
    /* same, different nmemb / size settings */
    rewind( fh );
    TESTCASE( memset( buffer, '\0', 20 ) == buffer );
    TESTCASE( fwrite( message, 5, 4, fh ) == 4 );
    rewind( fh );
    TESTCASE( fread( buffer, 5, 4, fh ) == 4 );
    TESTCASE( memcmp( buffer, message, 20 ) == 0 );
    TESTCASE( buffer[20] == 'x' );
    /* same... */
    rewind( fh );
    TESTCASE( memset( buffer, '\0', 20 ) == buffer );
    TESTCASE( fwrite( message, 20, 1, fh ) == 1 );
    rewind( fh );
    TESTCASE( fread( buffer, 20, 1, fh ) == 1 );
    TESTCASE( memcmp( buffer, message, 20 ) == 0 );
    TESTCASE( buffer[20] == 'x' );
    /* Done. */
    TESTCASE( fclose( fh ) == 0 );
    return TEST_RESULTS;
}
