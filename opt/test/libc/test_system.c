#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "punit.h"

static void setup(void)
{
}

static void teardown(void)
{
}

#define TESTSTRING "SUCCESS testing system()"
#define TESTFILE "/tmp/test_system.tmp"
static char * test_system(void)
{
    FILE * fh;
    char buffer[25];

    buffer[24] = 'x';
    pu_assert("", (fh = freopen(TESTFILE, "wb+", stdout)) != NULL);
    pu_assert("", system("echo " TESTSTRING " > " TESTFILE));
    rewind(fh);
    pu_assert("", fread(buffer, 1, 24, fh) == 24);
    pu_assert("", memcmp(buffer, TESTSTRING, 24) == 0);
    pu_assert("", buffer[24] == 'x');
    pu_assert("", fclose(fh) == 0);
    pu_assert("", remove(TESTFILE) == 0);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_system, PU_SKIP);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
