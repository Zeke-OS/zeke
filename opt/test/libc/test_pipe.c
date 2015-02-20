#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include "punit.h"

static void setup()
{
}

static void teardown()
{
}

static char * test_simple()
{
    int p[2];
    char str[100];
#define TEST_STRING "testing"

    memset(str, '\0', sizeof(str));

    pu_assert_equal("pipe creation ok", pipe(p), 0);

    pu_assert("sane p[0]", p[0] > 0);
    pu_assert("sane p[1]", p[1] > 0);

    write(p[1], TEST_STRING, sizeof(TEST_STRING));
    pu_assert("read() ok", read(p[0], str, sizeof(TEST_STRING)) ==
            sizeof(TEST_STRING));
    pu_assert_str_equal("read string equals written", str, TEST_STRING);

#undef TEST_STRING

    return NULL;
}

static void all_tests()
{
    pu_run_test(test_simple);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

