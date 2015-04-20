/*
 * wcsstr( const wchar_t *, const wchar_t * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <wchar.h>

wchar_t * wcsstr(const wchar_t * s1, const wchar_t * s2)
{
    const wchar_t * p1 = s1;

    while (*s1) {
        const wchar_t * p2 = s2;

        while (*p2 && (*p1 == *p2)) {
            ++p1;
            ++p2;
        }

        if (!*p2) {
            return (wchar_t *) s1;
        }
        ++s1;
        p1 = s1;
    }

    return NULL;
}
