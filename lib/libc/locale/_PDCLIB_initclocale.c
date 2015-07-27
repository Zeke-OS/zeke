/*
 * _PDCLIB_initclocale( locale_t )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include "_PDCLIB_clocale.h"
#include <sys/_PDCLIB_locale.h>

void _PDCLIB_initclocale(locale_t l)
{
    // TODO: There will be more added here...

    l->_WCType     = _PDCLIB_wcinfo;
    l->_WCTypeSize = _PDCLIB_wcinfo_size;
}
