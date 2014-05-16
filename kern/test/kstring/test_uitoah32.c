/**
 * @file test_uitoah32.c
 * @brief Test uitoah32.
 */

#include <stdint.h>
#include <kunit.h>
#include <test/ktest_mib.h>
#include <kstring.h>

#define NTOSTR(s) _NTOSTR(s)
#define _NTOSTR(s) #s

static void setup()
{
}

static void teardown()
{
}

static char * test_uitoah32(void)
{
#define UINTHEXVAL 0x0000532a
    char actual[80];
    char expected[] = NTOSTR(UINTHEXVAL);
    size_t retVal;

    retVal = uitoah32(actual, (uint32_t)UINTHEXVAL);

    ku_assert_str_equal("Unsigned integer was converted to string.", \
            actual, expected);
    ku_assert_equal("return value is number of printable characters in the string.", \
            (int)retVal, (int)(sizeof(expected) - 1));

#undef UINTVAL

    return 0;
}

static void all_tests() {
    ku_def_test(test_uitoah32, KU_RUN);
}

SYSCTL_TEST(kstring, uitoah32);
