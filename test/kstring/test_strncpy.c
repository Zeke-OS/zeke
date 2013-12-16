/**
 * @file test_strncpy.c
 * @brief Test strncpy.
 */

#include <stdio.h>
#include <punit.h>

char * strncpy(char * dst, const char * src, size_t n);

static void setup()
{
}

static void teardown()
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

    pu_assert_str_equal("String was copied correctly", str2, str1);
    pu_assert_equal("Limit was respected", str2[sizeof(str2)-1], '\0');
    pu_assert_equal("Limit was respected", str2[sizeof(str2)-2], 'X');

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static char * test_strncpy_2(void)
{
#define TSTRING1 "Y"
#define TSTRING2 "XXXX"
#define TCSTRING TSTRING1 TSTRING2
    char str1[] = TSTRING1;
    char str2[] = TSTRING2;

    strncpy(str2, str1, sizeof(str1) + 1);

    pu_assert_str_equal("String was copied correctly", str2, str1);
    pu_assert_equal("Limit was respected", str2[sizeof(str2)-1], '\0');
    pu_assert_equal("Limit was respected", str2[sizeof(str2)-2], 'X');
    pu_assert_equal("One byte was cleared", str2[sizeof(str2)-3], '\0');

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static void all_tests() {
    pu_def_test(test_strncpy_1, PU_RUN);
    pu_def_test(test_strncpy_2, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

