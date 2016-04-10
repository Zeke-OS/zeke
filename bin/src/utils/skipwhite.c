#include <ctype.h>

char * util_skipwhite(char * s)
{
    while (isspace(*s)) {
        ++s;
    }

    return s;
}
