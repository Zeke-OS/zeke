/**
 * @file test_strcpy.c
 * @brief Test strcpy.
 */

#include <kunit.h>
#include <ktest_mib.h>
#include <kstring.h>

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_strcpy(void)
{
#define TSTRING1 "YY"
#define TSTRING2 "XXXX"
#define TCSTRING TSTRING1 TSTRING2
    char str1[] = TSTRING1;
    char str2[] = TSTRING2;

    strncpy(str2, str1, sizeof(str1));

    ku_assert_str_equal("String was copied correctly", str2, str1);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_strcpy, KU_RUN);
}

SYSCTL_TEST(kstring, strcpy);
