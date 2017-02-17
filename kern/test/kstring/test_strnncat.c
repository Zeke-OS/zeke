/**
 * @file test_strnncat.c
 * @brief Test strnncat.
 */

#include <kunit.h>
#include <kstring.h>

static void setup(void)
{
}

static void teardown(void)
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

    ku_assert_str_equal("Strings were concatenated correctly", str1, TCSTRING);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static char * test_strnncat_same_array(void)
{
#define TSTRING "string1"
#define TCSTRING TSTRING TSTRING
    char str[20] = TSTRING;

    strnncat(str, 20, str, 20);

    ku_assert_str_equal("String can be concatenated with it self.", str,
                        TCSTRING);

#undef TSTRING
#undef TCSTRING

    return NULL;
}

static char * test_strnncat_limit1(void)
{
#define TSTRING1 "string1"
#define TSTRING2 "string2"
    char str1[40] = TSTRING1;
    char str2[] = TSTRING2;

    strnncat(str1, 8, str2, 7);

    ku_assert_str_equal("Strings were concatenated correctly", str1, TSTRING1);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static char * test_strnncat_limit2(void)
{
#define TSTRING1 "string1"
#define TSTRING2 "string2"
#define TCSTRING "string1str"
    char str1[20] = TSTRING1;
    char str2[] = TSTRING2;

    strnncat(str1, 11, str2, 4);

    ku_assert_str_equal("Strings were concatenated correctly", str1, TCSTRING);

#undef TSTRING1
#undef TSTRING2
#undef TCSTRING

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_strnncat_two_strings, KU_RUN);
    ku_def_test(test_strnncat_same_array, KU_RUN);
    ku_def_test(test_strnncat_limit1, KU_RUN);
    ku_def_test(test_strnncat_limit2, KU_RUN);
}

TEST_MODULE(kstring, strnncat);
