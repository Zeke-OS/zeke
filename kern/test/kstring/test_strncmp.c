/**
 * @file test_strncmp.c
 * @brief Test strncmp.
 */

#include <kunit.h>
#include <ktest_mib.h>
#include <kstring.h>

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

    ku_assert_equal("Strings are equal", retval, 0);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static void all_tests() {
    ku_def_test(test_strncmp, KU_RUN);
}

SYSCTL_TEST(kstring, strncmp);
