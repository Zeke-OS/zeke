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
#define TSTRING1    "string1"
#define UINTVAL     13
#define TSTRINGF TSTRING1 NTOSTR(UINTVAL)
    char dest[80];

    ksprintf(dest, sizeof(dest), TSTRING1 "%u", (uint32_t)UINTVAL);

    pu_assert_str_equal("String composed correctly.", dest, TSTRINGF);

#undef TSTRING1
#undef UINTVAL1
#undef TSTRINGF
}

static char * test_hex(void)
{
#define TSTRING1    "string1"
#define HEXVALUE    0x00000500
#define TSTRINGF TSTRING1 NTOSTR(HEXVALUE)
    char dest[80];

    ksprintf(dest, sizeof(dest), TSTRING1 "%x", (uint32_t)HEXVALUE);

    pu_assert_str_equal("String composed correctly.", dest, TSTRINGF);

#undef TSTRING1
#undef HEXVALUE
#undef TSTRINGF

    return 0;
}

static char * test_char(void)
{
#define TSTRING     "TEXT1"
#define TCHAR       'c'
    char dest[80];
    char final[] = TSTRING;
    final[strlen(final)] = TCHAR;

    ksprintf(dest, sizeof(dest), TSTRING "%c", TCHAR);

    pu_assert_str_equal("Strings were concatenated correctly", dest, final);

#undef TSTRING
#undef TCHAR

    return 0;
}

static char * test_string(void)
{
#define TSTRING1    "TEXT1"
#define TSTRING2    "TEXT2"
#define TSTRINGF TSTRING1 TSTRING2 TSTRING1
    char dest[80];
    char final[] = TSTRINGF;

    ksprintf(dest, sizeof(dest), TSTRING1 "%s" TSTRING1, TSTRING2);

    pu_assert_str_equal("Strings were concatenated correctly", dest, final);

#undef TSTRING1
#undef TSTRING2
#undef TSTRINGF

    return 0;
}

static char * test_percent(void)
{
#define TSTRING     "TEXT1"
    char dest[80];
    char final[] = "%" TSTRING "%";

    ksprintf(dest, sizeof(dest), "%%" TSTRING "%%");

    pu_assert_str_equal("Strings were concatenated correctly", dest, final);

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

