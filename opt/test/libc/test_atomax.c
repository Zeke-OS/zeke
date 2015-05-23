#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_PDCLIB_int.h>
#include "punit.h"

static void setup(void)
{
}

static void teardown(void)
{
}

char * test_atomax(void)
{
    /* basic functionality */
    pu_assert("", _PDCLIB_atomax("123") == 123);
    /* testing skipping of leading whitespace and trailing garbage */
    pu_assert("", _PDCLIB_atomax(" \n\v\t\f123xyz") == 123);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_atomax, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
