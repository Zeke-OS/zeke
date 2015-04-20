/*
 * wcscspn( const wchar_t *, const wchar_t * )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <wchar.h>

size_t wcscspn(const wchar_t * s1, const wchar_t * s2)
{
    size_t len = 0;

    while (s1[len]) {
        const wchar_t * p = s2;

        while (*p) {
             if (s1[len] == *p++)
                return len;
        }
        ++len;
    }

    return len;
}
