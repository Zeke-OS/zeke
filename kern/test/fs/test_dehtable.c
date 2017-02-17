/**
 * @file test_dehtable.c
 * @brief Test directory entry hash table.
 */

#include <kunit.h>
#include <kmalloc.h>
#include <fs/fs.h>
#include <fs/dehtable.h>

#define get_dirent(dea, offset) ((dh_dirent_t *)((size_t)(dea) + (offset)))

static dh_table_t table;

static void setup(void)
{
    memset(table, 0, sizeof(dh_table_t));
}

static void teardown(void)
{
}

static char * test_link(void)
{
#define str "test"
    size_t i;
    vnode_t vnode;
    dh_dirent_t * dea;

    ku_test_description("Test that dh_link works correctly.");

    vnode.vn_num = 10;
    dh_link(&table, vnode.vn_num, 0, str);

    for (i = 0; i < DEHTABLE_SIZE; i++) {
        if ((dea = table[i]) != 0) break;
    }
    ku_assert("Created chain found.", dea != 0);

    ku_assert_equal("Entry has a correct vnode number.",
                    (int)dea[0].dh_ino, (int)vnode.vn_num);

#undef str
    return NULL;
}

static char * test_link_chain(void)
{
#define str1 "test"
#define str2 "teest"
    size_t i;
    int res;
    vnode_t vnode1;
    vnode_t vnode2;
    dh_dirent_t * dea;
    size_t offset;

    ku_test_description("Test that dh_link chaining works correctly.");

    vnode1.vn_num = 10;
    vnode2.vn_num = 11;

    res = dh_link(&table, vnode1.vn_num, 0, str1);
    ku_assert_equal("Insert succeeded.", res, 0);
    res = dh_link(&table, vnode2.vn_num, 0, str2);
    ku_assert_equal("Insert succeeded.", res, 0);

    for (i = 0; i < DEHTABLE_SIZE; i++) {
        if ((dea = table[i]) != 0) break;
    }
    ku_assert("Created chain found.", dea != 0);

    ku_assert_equal("First entry has a correct vnode number.",
                    (int)get_dirent(dea, 0)->dh_ino, (int)vnode1.vn_num);
    offset = get_dirent(dea, 0)->dh_size;
    ku_assert_equal("Second entry has a correct vnode number.",
                    (int)get_dirent(dea, offset)->dh_ino, (int)vnode2.vn_num);

#undef str1
#undef str2
    return NULL;
}

static char * test_lookup(void)
{
#define str1 "dest"
#define str2 "deest"
    int res;
    vnode_t vnode1;
    vnode_t vnode2;
    ino_t nnum;

    ku_test_description("Test that dh_lookup can locate the correct link.");

    vnode1.vn_num = 10;
    vnode2.vn_num = 11;

    res = dh_link(&table, vnode1.vn_num, 0, str1);
    ku_assert_equal("Insert succeeded.", res, 0);
    res = dh_link(&table, vnode2.vn_num, 0, str2);
    ku_assert_equal("Insert succeeded.", res, 0);

    ku_assert_equal("No error",
            dh_lookup(&table, str2, &nnum), 0);
    ku_assert_equal("vnode num equal.", (int)nnum, (int)vnode2.vn_num);

#undef str1
#undef str2
    return NULL;
}

static char * test_iterator(void)
{
#define str1 "ff"
#define str2 "fff"
#define str3 "file1"
#define str4 "file2"

    vnode_t vnode1;
    vnode_t vnode2;
    vnode_t vnode3;
    vnode_t vnode4;

    ku_test_description("Test that dirent hash table iterator works correctly.");

    /* Some vnodes */
    vnode1.vn_num = 0;
    vnode2.vn_num = 1;
    vnode3.vn_num = 2;
    vnode4.vn_num = 3;

    /* Insert entries */
    ku_assert_equal("Insert OK.",
            dh_link(&table, vnode1.vn_num, 0, str1), 0);
    ku_assert_equal("Insert OK.",
            dh_link(&table, vnode2.vn_num, 0, str2), 0);
    ku_assert_equal("Insert OK.",
            dh_link(&table, vnode3.vn_num, 0, str3), 0);
    ku_assert_equal("Insert OK.",
            dh_link(&table, vnode4.vn_num, 0, str4), 0);

    /* Actual test */
    {
        size_t i;
        dh_dir_iter_t it; /* dirent hash table iterator. */
        ino_t fnd_inodes[4] = {0, 0, 0, 0};

        /* Get an iterator */
        it = dh_get_iter(&table);

        /* Loop the iterator */
        for (i = 0; i < DEHTABLE_SIZE; i++) {
            dh_dirent_t * entry = dh_iter_next(&it);

            if (!entry)
                break;
            ku_assert("inode number is not larger than the largest given inode number.",
                      entry->dh_ino < 4);
            fnd_inodes[entry->dh_ino]++;
        }
        ku_assert_equal("Found 4 entries with the iterator.", i, 4);
        for (i = 0; i < 4; i++) {
            ku_assert_equal("Found every inode once.", (int)(fnd_inodes[i]), 1);
        }
    }

    return NULL;
}

static void all_tests(void)
{
    ku_def_test(test_link, KU_RUN);
    ku_def_test(test_link_chain, KU_SKIP);
    ku_def_test(test_lookup, KU_RUN);
    ku_def_test(test_iterator, KU_RUN);
}

TEST_MODULE(fs, dehtable);
