/**
 * @file test_strncmp.c
 * @brief Test strncmp.
 */

#include <stdio.h>
#include <punit.h>

int strncmp(const char * str1, const char * str2, size_t n);

static void setup()
{
}

static void teardown()
{
}

static char * test_strncmp(void)
{
#define TSTRING1 "YY"
#define TSTRING2 "YYXX"
#define TCSTRING TSTRING1 TSTRING2
    char str1[] = TSTRING1;
    char str2[] = TSTRING2;
    int retval;

    retval = strncmp(str1, str2, sizeof(str1) - 1);

    pu_assert_equal("Strings are equal", retval, 0);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static void all_tests() {
    pu_def_test(test_strncmp, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

