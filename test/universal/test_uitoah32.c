/**
 * @file test_uitoah32.c
 * @brief Test uitoah32.
 */

#include <stdio.h>
#include <stdint.h>
#include <punit.h>

#define NTOSTR(s) _NTOSTR(s)
#define _NTOSTR(s) #s

int uitoah32(char * str, uint32_t value);

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

    pu_assert_str_equal("Unsigned integer was converted to string.", actual, expected);
    pu_assert_equal("return value is number of printable characters in the string.", retVal, sizeof(expected) - 1);

#undef UINTVAL

    return 0;
}

static void all_tests() {
    pu_def_test(test_uitoah32, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

