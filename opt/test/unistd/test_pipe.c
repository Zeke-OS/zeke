#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include "punit.h"

static int fd[2];

static void setup(void)
{
}

static void teardown(void)
{
    if (fd[0] > 0)
        close(fd[0]);
    fd[0] = 0;

    if (fd[1] > 0)
        close(fd[1]);
    fd[1] = 0;
}

static char * test_simple(void)
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

static char * test_eof(void)
{
    char str[] = "testing";

    pu_assert_equal("pipe creation ok", pipe(fd), 0);

    pu_assert("sane fd[0]", fd[0] > 0);
    pu_assert("sane fd[1]", fd[1] > 0);

    write(fd[1], str, sizeof(str));
    read(fd[0], str, sizeof(str));
    close(fd[1]);

    pu_assert("EOF returned", read(fd[0], str, sizeof(str)) == EOF);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_simple, PU_RUN);
    pu_def_test(test_eof, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

