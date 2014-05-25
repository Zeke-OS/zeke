/**
 * @file test_inpool.c
 * @brief Test inode pool.
 */

#include <kunit.h>
#include <ktest_mib.h>
#include <libkern.h>
#include <kmalloc.h>
#include <fs/fs.h>
#include "../../fs/inpool.h"

typedef struct inode {
    vnode_t in_vnode;
    int data;
} inode_t;

static vnode_t * create_tst(const fs_superblock_t * sb, ino_t * num);
static int delete_tst(vnode_t * vnode);

fs_superblock_t sb_tst = {
   .delete_vnode = delete_tst
};

static void setup()
{
}

static void teardown()
{
}

static char * test_inpool_init(void)
{
    inpool_t pool;
#define POOL_MAX 10

    ku_test_description("Test that inpool_init initializes the inode pool struct correctly.");

    inpool_init(&pool, &sb_tst, create_tst, POOL_MAX);

    ku_assert("Pool array is initialized", pool.ip_arr != 0);
    ku_assert_ptr_equal("Pool array contains some inodes", pool.ip_arr[0]->sb, &sb_tst);
    ku_assert_ptr_equal("Pool array contains some inodes", pool.ip_arr[3]->sb, &sb_tst);

#undef POOL_MAX

    return 0;
}

static char * test_inpool_destroy(void)
{
    inpool_t pool;

    ku_test_description("Test that inode pool is destroyed correctly.");

    inpool_init(&pool, &sb_tst, create_tst, 5);
    inpool_destroy(&pool);

    ku_assert_equal("Pool max size is set to zero.", pool.ip_max, 0);
    ku_assert_ptr_equal("Pool array pointer is set to null.", pool.ip_arr, (void *)0);

    return 0;
}

static char * test_inpool_get(void)
{
    inpool_t pool;
    vnode_t * vnode;
    inode_t * inode;
    size_t old_value;
#define POOL_MAX 10

    ku_test_description("Test that it's possible to get inodes from the pool.");

    inpool_init(&pool, &sb_tst, create_tst, POOL_MAX);

    old_value = pool.ip_rd;
    vnode = inpool_get_next(&pool);
    ku_assert("Got vnode", vnode != 0);
    ku_assert("Rd index was updated", pool.ip_rd != old_value);

    inode = container_of(vnode, inode_t, in_vnode);
    ku_assert_ptr_equal("sb is set", inode->in_vnode.sb, &sb_tst);
    ku_assert_equal("Preset data is ok", inode->data, 16);

#undef POOL_MAX
    return 0;
}

static char * test_inpool_insert(void)
{
    inpool_t pool;
    vnode_t * vnode;
    vnode_t * vnode1;

    ku_test_description("Test that inode recycling works correctly.");

    inpool_init(&pool, &sb_tst, create_tst, 1);
    vnode = inpool_get_next(&pool);
    ku_assert("Got vnode", vnode != 0);
    inpool_insert(&pool, vnode);
    vnode1 = inpool_get_next(&pool);
    ku_assert_ptr_equal("Got same vnode", vnode1, vnode);

    return 0;
}

static void all_tests() {
    ku_def_test(test_inpool_init, KU_RUN);
    ku_def_test(test_inpool_destroy, KU_RUN);
    ku_def_test(test_inpool_get, KU_RUN);
    ku_def_test(test_inpool_insert, KU_RUN);
}

SYSCTL_TEST(fs, inpool);

static vnode_t * create_tst(const fs_superblock_t * sb, ino_t * num)
{
    inode_t * inode;

    inode = kcalloc(1, sizeof(inode_t));
    if (inode == 0) {
        return 0;
    }

    inode->in_vnode.vnode_num = *num;
    inode->in_vnode.refcount = 0;
    inode->in_vnode.sb = &sb_tst;
    inode->data = 16;

    return &(inode->in_vnode);
}

static int delete_tst(vnode_t * vnode)
{
    kfree(container_of(vnode, inode_t, in_vnode));
    return 0;
}
