#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <limits.h>
#include "punit.h"

char _oldname[HOST_NAME_MAX + 1];

static void setup()
{
    gethostname(_oldname, sizeof(_oldname));
}

static void teardown()
{
    sethostname(_oldname, strlen(_oldname) + 1);
}

static char * test_gethostname()
{
    char name[HOST_NAME_MAX + 1];

    pu_assert_equal("gethostname() works", gethostname(name, sizeof(name)), 0);

    return NULL;
}

static char * test_sethostname_valid()
{
    char newname[] = "new-valid1-hostname";
    char name[HOST_NAME_MAX + 1];
    size_t newnamelen = strlen(newname) + 1;

    pu_assert_equal("sethostname() with a valid hostname",
                    sethostname(newname, newnamelen), 0);
    pu_assert_equal("get new hostname", gethostname(name, sizeof(name)), 0);
    pu_assert_str_equal("hostname matches", name, newname);

    return NULL;
}

static char * test_sethostname_invalid1()
{
    char newname[] = "1new";
    size_t newnamelen = strlen(newname) + 1;

    pu_assert_equal("sethostname() fails with a invalid hostname",
              sethostname(newname, newnamelen), -1);

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_gethostname, PU_RUN);
    pu_def_test(test_sethostname_valid, PU_RUN);
    pu_def_test(test_sethostname_invalid1, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

