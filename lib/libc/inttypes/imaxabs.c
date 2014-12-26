/* imaxabs( intmax_t )
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <inttypes.h>

intmax_t imaxabs(intmax_t j)
{
    return (j >= 0) ? j : -j;
}
