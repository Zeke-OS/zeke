/* vsnprintf( char *, size_t, const char *, va_list ap )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <stdio.h>
#include <stdarg.h>

#include <sys/_PDCLIB_io.h>
#include <string.h>

struct state {
    size_t bufrem;
    char *bufp;
};

static size_t strout( void *p, const char *buf, size_t sz )
{
    struct state *s = p;
    size_t copy = s->bufrem >= sz ? sz : s->bufrem;

    memcpy( s->bufp, buf, copy );
    s->bufrem -= copy;
    s->bufp   += copy;

    return sz;
}

int vsnprintf( char * _PDCLIB_restrict s,
               size_t n,
               const char * _PDCLIB_restrict format,
               _PDCLIB_va_list arg )
{
    struct state st;
    st.bufrem = n;
    st.bufp   = s;
    int r = _vcbprintf( &st, strout, format, arg );
    if ( st.bufrem )
    {
        *st.bufp = 0;
    }

    return r;
}
