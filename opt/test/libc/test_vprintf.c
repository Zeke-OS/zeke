#define _PDCLIB_FILEID "stdio/vprintf.c"
#define _PDCLIB_FILEIO
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include "punit.h"

const char * testfile = "test.fil";

static void setup(void)
{
}

static void teardown(void)
{
}

static int testprintf(FILE * stream, const char * format, ...)
{
    int i;
    va_list arg;
    va_start(arg, format);
    i = vprintf(format, arg);
    va_end(arg);
    return i;
}

char * test_vprintf(void)
{
    FILE * target;

    pu_assert("",  (target = freopen(testfile, "wb+", stdout)) != NULL);
    /* TODO vprintf test cases */
/*#include "printf_testcases.h"*/
    pu_assert("",  fclose(target) == 0);
    pu_assert("",  remove(testfile) == 0);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_vprintf, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}

