/**
 * @file test_strncpy.c
 * @brief Test strncpy.
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

static char * test_strncpy_1(void)
{
#define TSTRING1 "YY"
#define TSTRING2 "XXXX"
#define TCSTRING TSTRING1 TSTRING2
    char str1[] = TSTRING1;
    char str2[] = TSTRING2;

    strncpy(str2, str1, sizeof(str1));

    ku_assert_str_equal("String was copied correctly", str2, str1);
    ku_assert_equal("Limit was respected", str2[sizeof(str2)-1], '\0');
    ku_assert_equal("Limit was respected", str2[sizeof(str2)-2], 'X');

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static char * test_strncpy_2(void)
{
#define TSTRING1 "Y"
#define TSTRING2 "XXXX"
#define TCSTRING TSTRING1 TSTRING2
    char str1[] = TSTRING1;
    char str2[] = TSTRING2;

    strncpy(str2, str1, sizeof(str1) + 1);

    ku_assert_str_equal("String was copied correctly", str2, str1);
    ku_assert_equal("Limit was respected", str2[sizeof(str2)-1], '\0');
    ku_assert_equal("Limit was respected", str2[sizeof(str2)-2], 'X');
    ku_assert_equal("One byte was cleared", str2[sizeof(str2)-3], '\0');

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_strncpy_1, KU_RUN);
    ku_def_test(test_strncpy_2, KU_RUN);
}

SYSCTL_TEST(kstring, strncpy);
