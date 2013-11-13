/**
 * @file test_dtree.c
 * @brief Test dtree.
 */

#include <stdio.h>
#include <punit.h>
#include <fs/dtree.h>

void dtree_destroy_node(dtree_node_t * node);

static void setup()
{
    dtree_root.fname = "/";
    dtree_root.parent = &dtree_root;
    dtree_root.persist = 1;
}

static void teardown()
{
    memset((void *)(&dtree_root), 0, sizeof(dtree_node_t));
}

static char * test_path_compare(void)
{
    //size_t path_compare(char * fname, char * path, size_t offset)

    pu_assert("Path equals", path_compare("base", "/base/node", 1) != 0);
    pu_assert("Path equals", path_compare("child", "/base/node/child", 11) != 0);

    return 0;
}

static char * test_create(void)
{
    dtree_node_t * node;
    dtree_node_t * tnode;
    int i;

    node = dtree_create_node(&dtree_root, "var", 0);

    pu_assert_ptr_equal("Correct node parent.", node->parent, &dtree_root);
    pu_assert_str_equal("Correct name.", node->fname, "var");
    for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
        if (dtree_root.child[i] != 0) {
            tnode = dtree_root.child[i];
        }
        if (dtree_root.pchild[i] != 0 && i > 0) {
            pu_assert_fail("There should be no persisted childs yet.");
        }
    }
    pu_assert_ptr_equal("New node is a child of root", tnode, node);

    dtree_destroy_node(node);

    return 0;
}

static char * test_remove()
{
    dtree_node_t * node1;
    dtree_node_t * node2;
    dtree_node_t * node3;
    dtree_node_t * retval;
    int i;

    node1 = dtree_create_node(&dtree_root, "usr", 0);
    node2 = dtree_create_node(node1, "ab", 1);
    node3 = dtree_create_node(node1, "cd", 0);

    retval = dtree_lookup("/usr/cd");
    pu_assert_ptr_equal("Got cd node", retval, node3);

    for (i = 0; i < 2; i++) {
        retval = dtree_lookup("/usr/ab");
        pu_assert_ptr_equal("Got ab node", retval, node2);
        pu_assert_str_equal("Name equals expected", retval->fname, "ab");

        retval = dtree_lookup("/usr/cd");
        pu_assert("Got some node for non-persistent cache entry", retval != 0);

        dtree_remove_node(node1, 0);
        printf("\tPass %i\n", i);
    }

    return 0;
}

static char * test_collision()
{
    dtree_node_t * node1;
    dtree_node_t * node2;
    dtree_node_t * node3;
    dtree_node_t * retval;
    int i;

    node1 = dtree_create_node(&dtree_root, "usr", 0);
    node2 = dtree_create_node(node1, "ab", 0);

    retval = dtree_lookup("/usr/ab");
    pu_assert_ptr_equal("Got ab node", retval, node2);

    node3 = dtree_create_node(node1, "aab", 0);

    retval = dtree_lookup("/usr/ab");
    pu_assert("No more ab node", retval != node2);

    retval = dtree_lookup("/usr/aab");
    pu_assert_ptr_equal("Got aab node", retval, node3);

    return 0;
}

static void all_tests() {
    pu_def_test(test_create, PU_RUN);
    pu_def_test(test_path_compare, PU_RUN);
    pu_def_test(test_remove, PU_RUN);
    pu_def_test(test_collision, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

