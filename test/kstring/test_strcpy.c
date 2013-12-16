/**
 * @file test_strcpy.c
 * @brief Test strcpy.
 */

#include <stdio.h>
#include <punit.h>

char * strcpy(char * dst, const char * src);

static void setup()
{
}

static void teardown()
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

    pu_assert_str_equal("String was copied correctly", str2, str1);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static void all_tests() {
    pu_def_test(test_strcpy, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

