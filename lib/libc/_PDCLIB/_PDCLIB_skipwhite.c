#include <ctype.h>

char * _PDCLIB_skipwhite(char * s)
{
    while (isspace(*s)) {
        ++s;
    }

    return s;
}
