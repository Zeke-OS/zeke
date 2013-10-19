/**
 * @file test_uitoa32.c
 * @brief Test uitoa32.
 */

#include <stdio.h>
#include <stdint.h>
#include <punit.h>

#define NTOSTR(s) _NTOSTR(s)
#define _NTOSTR(s) #s

int uitoa32(char * str, uint32_t value);

static void setup()
{
}

static void teardown()
{
}

static char * test_uitoa32(void)
{
#define UINTVAL 1337
    char actual[80];
    char expected[] = NTOSTR(UINTVAL);
    size_t retVal;

    retVal = uitoa32(actual, (uint32_t)UINTVAL);

    pu_assert_str_equal("Unsigned integer was converted to string.", actual, expected);
    pu_assert_equal("return value is number of printable characters in the string.", (int)retVal, (int)(sizeof(expected) - 1));

#undef UINTVAL

    return 0;
}

static void all_tests() {
    pu_def_test(test_uitoa32, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

