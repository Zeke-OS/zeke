/**
 * @file test_bio.c
 * @brief Test buffered IO.
 */

#include <fcntl.h>
#include <buf.h>
#include <fs/fs.h>
#include <ktest_mib.h>
#include <kunit.h>
#include <libkern.h>
#include <proc.h>

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_geteblk(void)
{
    struct buf * bp;

    ku_test_description("Test that geteblk() returns a valid buffer.");

    bp = geteblk(4096);

    ku_assert("A new buffer was returned", bp);
    ku_assert("Buf size is correct", bp->b_bufsize >= 4096);
    ku_assert_equal("Buf requested size is correct", bp->b_bcount, 4096);
    ku_assert("Corrects flags are set", bp->b_flags & B_BUSY);

    brelse(bp);

    return NULL;
}

static char * test_getblk(void)
{
    vnode_t * vndev;
    struct buf * bp;
    struct proc_info * proc;

    ku_test_description("Test that getblk() returns a device backed buffer.");

    proc = proc_ref(0, PROC_NOT_LOCKED);
    proc_unref(proc);

    ku_assert("lookup failed",
               !lookup_vnode(&vndev, proc->croot, "dev/zero",
                             O_RDWR));
    bp = getblk(vndev, 0, 4096, 0);
    ku_assert("got a buffer", bp);

    ku_assert("bp is marked as busy", bp->b_flags & B_BUSY);

    brelse(bp);

    return NULL;
}

static char * test_bread(void)
{
    vnode_t * vndev;
    struct buf * bp;
    struct proc_info * proc;
    int err;

    ku_test_description("Test that bread() reads.");

    proc = proc_ref(0, PROC_NOT_LOCKED);
    proc_unref(proc);

    ku_assert("lookup failed",
              !lookup_vnode(&vndev, proc->croot,
                            "/dev/zero", O_RDWR));
    err = bread(vndev, 0, 4096, &bp);
    ku_assert_equal("no error", err, 0);
    ku_assert("got a buffer", bp);

    /* Verify read */
    for (int i = 0; i < 4096; i += 512) {
        ku_assert_equal("SBZ", ((uint8_t *)bp->b_data)[i], 0);
    }

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_geteblk, KU_RUN);
    ku_def_test(test_getblk, KU_RUN);
    ku_def_test(test_bread, KU_SKIP);
}

SYSCTL_TEST(vm, bio);
