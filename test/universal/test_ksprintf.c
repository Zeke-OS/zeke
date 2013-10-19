/**
 * @file test_ksprintf.c
 * @brief Test ksprintf.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <punit.h>

#define NTOSTR(s) _NTOSTR(s)
#define _NTOSTR(s) #s

#define JUNK "junkjunkjunkjunkjunkjunkjunkjunkjunkjunkjunkjunkjunkjunkjunkjunk"

/* These are needed by ksprintf */
int uitoa32(char * str, uint32_t value);
int uitoah32(char * str, uint32_t value);
char * strnncat(char * dst, size_t ndst, const char * src, size_t nsrc);
size_t strlenn(const char * str, size_t max);

void ksprintf(char * str, size_t maxlen, const char * format, ...);

static void setup()
{
}

static void teardown()
{
}

static char * test_uint(void)
{
#define TSTRING1    "string"
#define UINTVAL     1337
#define EXPECTED TSTRING1 NTOSTR(UINTVAL) TSTRING1
    char actual[80] = JUNK;

    ksprintf(actual, sizeof(actual), TSTRING1 "%u" TSTRING1, (uint32_t)UINTVAL);

    pu_assert_str_equal("String composed correctly.", actual, EXPECTED);

#undef TSTRING1
#undef UINTVAL1
#undef EXPECTED

    return 0;
}

static char * test_hex(void)
{
#define TSTRING1    "string"
#define HEXVALUE    0x00000500
#define EXPECTED TSTRING1 NTOSTR(HEXVALUE) TSTRING1
    char actual[80] = JUNK;

    ksprintf(actual, sizeof(actual), TSTRING1 "%x" TSTRING1, (uint32_t)HEXVALUE);

    pu_assert_str_equal("String composed correctly.", actual, EXPECTED);

#undef TSTRING1
#undef HEXVALUE
#undef EXPECTED

    return 0;
}

static char * test_char(void)
{
#define TSTRING     "TEXT1"
#define TCHAR       'c'
    char actual[80] = JUNK;
    char final[] = TSTRING;
    final[strlen(final)] = TCHAR;

    ksprintf(actual, sizeof(actual), TSTRING "%c", TCHAR);

    pu_assert_str_equal("Strings were concatenated correctly", actual, final);

#undef TSTRING
#undef TCHAR

    return 0;
}

static char * test_string(void)
{
#define TSTRING1    "TEXT1"
#define TSTRING2    "TEXT2"
#define EXPECTED TSTRING1 TSTRING2 TSTRING1
    char actual[80] = JUNK;
    char expected[] = EXPECTED;

    ksprintf(actual, sizeof(actual), TSTRING1 "%s" TSTRING1, TSTRING2);

    pu_assert_str_equal("Strings were concatenated correctly", actual, expected);

#undef TSTRING1
#undef TSTRING2
#undef EXPECTED

    return 0;
}

static char * test_percent(void)
{
#define TSTRING     "TEXT1"
    char actual[80] = JUNK;
    char expected[] = "%" TSTRING "%";

    ksprintf(actual, sizeof(actual), "%%" TSTRING "%%");

    pu_assert_str_equal("Strings were concatenated correctly", actual, expected);

#undef TSTRING

    return 0;
}

static void all_tests() {
    pu_def_test(test_uint, PU_RUN);
    pu_def_test(test_hex, PU_RUN);
    pu_def_test(test_char, PU_RUN);
    pu_def_test(test_string, PU_RUN);
    pu_def_test(test_percent, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

