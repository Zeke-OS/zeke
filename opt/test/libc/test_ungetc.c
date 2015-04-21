
const char* hellostr = "Hello, world!";

int main( void )
{
    // Also see ftell() for some testing

    // PDCLIB-18: fread ignores ungetc
    size_t bufsz = strlen( hellostr ) + 1;
    char * buf = malloc( bufsz );
    FILE * fh;

    // Also fgets
    TESTCASE( ( fh = tmpfile() ) != NULL );
    TESTCASE( fputs(hellostr, fh) == 0 );
    rewind(fh);
    TESTCASE( fgetc( fh ) == 'H' );
    TESTCASE( ungetc( 'H', fh ) == 'H' );
    TESTCASE( fgets( buf, bufsz, fh ) != NULL );
    TESTCASE( strcmp( buf, hellostr ) == 0 );

    // fread
    rewind(fh);
    TESTCASE( fgetc( fh ) == 'H' );
    TESTCASE( ungetc( 'H', fh ) == 'H' );
    TESTCASE( fread( buf, bufsz - 1, 1, fh ) == 1 );
    TESTCASE( strncmp( buf, hellostr, bufsz - 1 ) == 0 );



    return TEST_RESULTS;
}
