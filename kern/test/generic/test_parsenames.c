#include <kunit.h>
#include <ktest_mib.h>
#include <libkern.h>
#include <kmalloc.h>

#define TEST_PATH       "test/file/path/"
#define TEST_FILENAME   "file.txt"

static char * path;
static char * filename;

static void setup(void)
{
}

static void teardown(void)
{
    kfree(path);
    kfree(filename);

    path = NULL;
    filename = NULL;
}

static char * test_parsenames_ok(void)
{
    char test_path[] = TEST_PATH TEST_FILENAME;
    int retval;

    retval = parsenames(test_path, &path, &filename);

    ku_assert_equal("Return value is 0", retval, 0);
    ku_assert_str_equal("Returned path is ok", path, TEST_PATH);
    ku_assert_str_equal("Returned filename is ok", filename, TEST_FILENAME);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_parsenames_ok, KU_RUN);
}

SYSCTL_TEST(generic, parsenames);
