/**
 * @file test_strnncat.c
 * @brief Test strnncat.
 */

#include <stdio.h>
#include <punit.h>

char * strnncat(char * dst, size_t ndst, char * src, size_t nsrc);

static void setup()
{
}

static void teardown()
{
}

static char * test_strnncat_two_strings(void)
{
#define TSTRING1 "string1"
#define TSTRING2 "string2"
#define TCSTRING TSTRING1 TSTRING2
    char str1[40] = TSTRING1;
    char str2[] = TSTRING2;

    strnncat(str1, 40, str2, 10);

    pu_assert_str_equal("Strings were concatenated correctly", str1, TCSTRING);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static char * test_strnncat_same_array(void)
{
#define TSTRING "string1"
#define TCSTRING TSTRING TSTRING
    char str[20] = TSTRING;

    strnncat(str, 20, str, 20);

    pu_assert_str_equal("String can be concatenated with it self.", str, TCSTRING);

#undef TSTRING
#undef TCSTRING

    return 0;
}

static char * test_strnncat_limit1(void)
{
#define TSTRING1 "string1"
#define TSTRING2 "string2"
    char str1[40] = TSTRING1;
    char str2[] = TSTRING2;

    strnncat(str1, 8, str2, 7);

    pu_assert_str_equal("Strings were concatenated correctly", str1, TSTRING1);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static char * test_strnncat_limit2(void)
{
#define TSTRING1 "string1"
#define TSTRING2 "string2"
#define TCSTRING "string1str"
    char str1[20] = TSTRING1;
    char str2[] = TSTRING2;

    strnncat(str1, 11, str2, 4);

    pu_assert_str_equal("Strings were concatenated correctly", str1, TCSTRING);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return 0;
}

static void all_tests() {
    pu_def_test(test_strnncat_two_strings, PU_RUN);
    pu_def_test(test_strnncat_same_array, PU_RUN);
    pu_def_test(test_strnncat_limit1, PU_RUN);
    pu_def_test(test_strnncat_limit2, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

