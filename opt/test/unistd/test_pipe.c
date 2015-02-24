#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include "punit.h"

static int fd[2];

static void setup()
{
}

static void teardown()
{
    if (fd[0] > 0)
        close(fd[0]);
    fd[0] = 0;

    if (fd[1] > 0)
        close(fd[1]);
    fd[1] = 0;
}

static char * test_simple()
{
    char str[100];
#define TEST_STRING "testing"

    memset(str, '\0', sizeof(str));

    pu_assert_equal("pipe creation ok", pipe(fd), 0);

    pu_assert("sane fd[0]", fd[0] > 0);
    pu_assert("sane fd[1]", fd[1] > 0);

    write(fd[1], TEST_STRING, sizeof(TEST_STRING));
    pu_assert("read() ok", read(fd[0], str, sizeof(TEST_STRING)) ==
            sizeof(TEST_STRING));
    pu_assert_str_equal("read string equals written", str, TEST_STRING);

#undef TEST_STRING

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_simple, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

