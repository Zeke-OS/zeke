#include <sys/sysctl.h>
#include <ktest_mib.h>
#include <kunit.h>
#include <libkern.h>

static int integer = 1;

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_add_rem_oid(void)
{
    struct sysctl_oid * oidp;
    int retval;

    oidp = sysctl_add_oid(&SYSCTL_NODE_CHILDREN(, debug),
                          "unittest", CTLTYPE_INT | CTLFLAG_RW, &integer, 0,
                          sysctl_handle_int, "I", "Integer");
    ku_assert("OID created", oidp != NULL);

    retval = sysctl_remove_oid(oidp, 1, 0);
    ku_assert_equal("OID removed", retval, 0);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_add_rem_oid, KU_RUN);
}

SYSCTL_TEST(generic, sysctl);
