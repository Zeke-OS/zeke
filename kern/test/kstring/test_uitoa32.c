/**
 * @file test_uitoa32.c
 * @brief Test uitoa32.
 */

#include <stdint.h>
#include <kunit.h>
#include <ktest_mib.h>
#include <kstring.h>

#define NTOSTR(s) _NTOSTR(s)
#define _NTOSTR(s) #s

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_uitoa32(void)
{
#define UINTVAL 1337
    char actual[80];
    char expected[] = NTOSTR(UINTVAL);
    size_t retVal;

    retVal = uitoa32(actual, (uint32_t)UINTVAL);

    ku_assert_str_equal("Unsigned integer was converted to string.", actual,
                        expected);
    ku_assert_equal("Returned the number of printable characters in the string.",
                    (int)retVal, (int)(sizeof(expected) - 1));

#undef UINTVAL

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_uitoa32, KU_RUN);
}

SYSCTL_TEST(kstring, uitoa32);
