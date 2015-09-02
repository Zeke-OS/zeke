/**
 * @file test_inpool.c
 * @brief Test inode pool.
 */

#include <kunit.h>
#include <ktest_mib.h>
#include <libkern.h>
#include <kmalloc.h>
#include <fs/fs.h>
#include <fs/inpool.h>

typedef struct inode {
    vnode_t in_vnode;
    int data;
} inode_t;

static vnode_t * create_tst(const struct fs_superblock * sb, ino_t * num);
static void delete_tst(vnode_t * vnode);
static int delete_tst_vnode(vnode_t * vnode)
{
    delete_tst(vnode);
    return 0;
}

struct fs_superblock sb_tst = {
   .delete_vnode = delete_tst_vnode,
};

static void setup(void)
{
}

static void teardown(void)
{
}

static char * test_inpool_init(void)
{
    inpool_t pool;
    int err;

    ku_test_description("Test that inpool_init initializes the inode pool struct correctly.");

    err = inpool_init(&pool, &sb_tst, create_tst, delete_tst, 10);
    ku_assert_equal("inpool created succesfully", err, 0);

    return NULL;
}

static char * test_inpool_destroy(void)
{
    inpool_t pool;

    ku_test_description("Test that inode pool is destroyed correctly.");

    inpool_init(&pool, &sb_tst, create_tst, delete_tst, 5);
    inpool_destroy(&pool);

    ku_assert_equal("Pool max size is set to zero.", pool.ip_max, 0);

    return NULL;
}

static char * test_inpool_get(void)
{
    inpool_t pool;
    vnode_t * vnode;
    inode_t * inode;

    ku_test_description("Test that it's possible to get inodes from the pool.");

    inpool_init(&pool, &sb_tst, create_tst, delete_tst, 10);

    vnode = inpool_get_next(&pool);
    ku_assert("Got vnode", vnode != 0);

    inode = container_of(vnode, inode_t, in_vnode);
    ku_assert_ptr_equal("sb is set", inode->in_vnode.sb, &sb_tst);
    ku_assert_equal("Preset data is ok", inode->data, 16);

    return NULL;
}

static char * test_inpool_insert(void)
{
    inpool_t pool;
    vnode_t * vnode;
    vnode_t * vnode1;

    ku_test_description("Test that inode recycling works correctly.");

    inpool_init(&pool, &sb_tst, create_tst, delete_tst, 10);
    vnode = inpool_get_next(&pool);
    ku_assert("Got vnode", vnode != 0);
    inpool_insert(&pool, vnode);
    vnode1 = inpool_get_next(&pool);
    ku_assert_ptr_equal("Got same vnode", vnode1, vnode);

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_inpool_init, KU_RUN);
    ku_def_test(test_inpool_destroy, KU_RUN);
    ku_def_test(test_inpool_get, KU_RUN);
    ku_def_test(test_inpool_insert, KU_RUN);
}

SYSCTL_TEST(fs, inpool);

static vnode_t * create_tst(const struct fs_superblock * sb, ino_t * num)
{
    inode_t * inode;

    inode = kcalloc(1, sizeof(inode_t));
    if (inode == 0) {
        return NULL;
    }

    inode->in_vnode.vn_num = *num;
    inode->in_vnode.vn_refcount = 0;
    inode->in_vnode.sb = &sb_tst;
    inode->data = 16;

    return &(inode->in_vnode);
}

static void delete_tst(vnode_t * vnode)
{
    kfree(container_of(vnode, inode_t, in_vnode));
}
