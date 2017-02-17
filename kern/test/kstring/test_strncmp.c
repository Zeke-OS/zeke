/**
 * @file test_strncmp.c
 * @brief Test strncmp.
 */

#include <kunit.h>
#include <kstring.h>

static void setup(void)
{
}

static void teardown(void)
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

    ku_assert_equal("Strings are equal", retval, 0);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_strncmp, KU_RUN);
}

TEST_MODULE(kstring, strncmp);
