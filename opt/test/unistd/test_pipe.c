#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
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
    fd[1] = 0;

    pu_assert("Nothing to read", read(fd[0], str, sizeof(str)) == 0);

    return NULL;
}

static char * test_eof_remaining(void)
{
    char str[] = "testing";

    pu_assert_equal("pipe creation ok", pipe(fd), 0);
    pu_assert("sane fd[0]", fd[0] > 0);
    pu_assert("sane fd[1]", fd[1] > 0);

    write(fd[1], str, sizeof(str));
    close(fd[1]);
    fd[1] = 0;
    pu_assert("read() ok", read(fd[0], str, sizeof(str)) == sizeof(str));

    return NULL;
}

static char * test_pipe_after_fork(void)
{
#define STR "testing"
    const char tstr[] = STR;
    pid_t pid;

    pu_assert_equal("pipe creation ok", pipe(fd), 0);
    pu_assert("sane fd[0]", fd[0] > 0);
    pu_assert("sane fd[1]", fd[1] > 0);

    pid = fork();
    pu_assert("PID OK\n", pid != -1);
    if (pid == 0) {
        close(fd[0]);
        write(fd[1], tstr, sizeof(tstr));

        _exit(0);
    } else {
        ssize_t n, i = 0;
        char str[sizeof(STR)] = "";

        close(fd[1]);
        while ((n = read(fd[0], str + i, 1))) {
            pu_assert("Only one char was read", n == 1);
            pu_assert_equal("Proper char was received", *(str + i), tstr[i]);
            if (++i == sizeof(str))
                break;
        }
    }

    wait(NULL);

#undef STR
    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_simple, PU_RUN);
    pu_def_test(test_eof, PU_RUN);
    pu_def_test(test_eof_remaining, PU_RUN);
    pu_def_test(test_pipe_after_fork, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

