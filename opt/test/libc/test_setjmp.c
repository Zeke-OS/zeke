#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <setjmp.h>
#include "punit.h"

static jmp_buf buf;
static int fd[2];

static void setup(void)
{
    int err;

    err = pipe(fd);
    if (err)
        _exit(1);
}

static void teardown(void)
{
    close(fd[0]);
    close(fd[1]);
}

void second(void)
{
    const char str[] = "second";

    write(fd[1], str, sizeof(str) - 1);
    longjmp(buf, 1);
}

void first(void)
{
    const char str[] = "first";

    second();
    write(fd[1], str, sizeof(str) - 1);
}

char * test_setjmp(void)
{
    const char str[] = "main";
    char strbuf[20];

    if (!setjmp(buf)) {
        first();
    } else {
        write(fd[1], str, sizeof(str));
    }

    read(fd[0], strbuf, 11);
    strbuf[10] = '\0';
    pu_assert_str_equal("Written string is ok", strbuf, "secondmain");

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_setjmp, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
