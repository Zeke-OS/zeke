/*
 * abort(void)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stdlib.h>
#include <signal.h>

void abort(void)
{
    raise(SIGABRT);
    exit(EXIT_FAILURE);
}
