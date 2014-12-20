/* getenv( const char * )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <string.h>
#include <stdlib.h>

extern char * * environ;

char * getenv( const char * name )
{
    size_t len = strlen( name );
    size_t index = 0;
    while ( environ[ index ] != NULL )
    {
        if ( strncmp( environ[ index ], name, len ) == 0 )
        {
            return environ[ index ] + len + 1;
        }
        index++;
    }
    return NULL;
}
