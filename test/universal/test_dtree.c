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
    dtree_init();
}

static void teardown()
{
    memset((void *)(&dtree_root), 0, sizeof(dtree_node_t));
}

static char * test_path_compare(void)
{
    pu_assert("Path equals", path_compare("base", "/base/node", 1) != 0);
    pu_assert("Path equals", path_compare("child", "/base/node/child", 11) != 0);

    return 0;
}

static char * test_create(void)
{
    dtree_node_t * node;
    dtree_node_t * tnode = 0;
    size_t i;

    node = dtree_create_node(&dtree_root, "var", 0);

    pu_assert_ptr_equal("Correct node parent.", node->parent, &dtree_root);
    pu_assert_str_equal("Correct name.", node->fname, "var");
    for (i = 0; i < DTREE_HTABLE_SIZE; i++) {
        if (dtree_root.child[i]->head != 0) {
            tnode = dtree_root.child[i]->head;
        }
    }
    pu_assert_ptr_equal("New node is a child of root", tnode, node);

    node = dtree_remove_node(node, DTREE_NODE_PERS);

    return 0;
}

static char * test_getpath()
{
    dtree_node_t * node1;
    dtree_node_t * node2;
    char * path;

    node1 = dtree_create_node(&dtree_root, "usr", 1);
    node2 = dtree_create_node(node1, "ab", 1);

    path = dtree_getpath(node2);
    pu_assert_str_equal("Path equals expected", path, "/usr/ab");
    free(path);

    path = dtree_getpath(&dtree_root);
    pu_assert_str_equal("Path equals expected", path, "/");
    free(path);

    return 0;
}

static char * test_lookup()
{
    dtree_node_t * node1;
    dtree_node_t * node2;
    dtree_node_t * retval;
    const char path[] = "/usr/ab";

    node1 = dtree_create_node(&dtree_root, "usr", 0);
    node2 = dtree_create_node(node1, "ab", 1);

    retval = dtree_lookup("/", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_str_equal("Got / node", retval->fname, "/");

    retval = dtree_lookup("/usr", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_str_equal("Got usr node", retval->fname, "usr");

    retval = dtree_lookup(path, DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_ptr_equal("Got ab node", retval, node2);
    pu_assert_str_equal("Name equals expected", retval->fname, "ab");

    dtree_remove_node(node1, DTREE_NODE_PERS);

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

    retval = dtree_lookup("/usr/cd", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_ptr_equal("Got cd node", retval, node3);
    dtree_discard_node(node3);

    for (i = 0; i < 2; i++) {
        retval = dtree_lookup("/usr/ab", DTREE_LOOKUP_MATCH_EXACT);
        pu_assert_ptr_equal("Got ab node", retval, node2);
        pu_assert_str_equal("Name equals expected", retval->fname, "ab");

        retval = dtree_lookup("/usr/cd", DTREE_LOOKUP_MATCH_ANY);
        pu_assert("Got some node for non-persistent cache entry", retval != 0);

        dtree_remove_node(node1, 0);
        printf("\tPass %i\n", i);
    }

    return 0;
}

static char * test_dicard()
{
    dtree_node_t * node1;
    dtree_node_t * retval;

    pu_test_description("Test if non persistent dtree node is flushed only after all references are discarded.");

    node1 = dtree_create_node(&dtree_root, "usr", 0);

    retval = dtree_lookup("/usr", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_ptr_equal("Got usr node", retval, node1);

    /* Try to flush */
    dtree_remove_node(&dtree_root, 0);

    retval = dtree_lookup("/usr", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_ptr_equal("Got usr node", retval, node1);

    /* Now we have incremented refcount twice, so... */
    dtree_discard_node(retval);
    dtree_discard_node(retval);

    dtree_remove_node(&dtree_root, 0);

    retval = dtree_lookup("/usr", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_ptr_equal("usr node is now flushed", retval, 0);

    return 0;
}

static char * test_collision()
{
    dtree_node_t * node1;
    dtree_node_t * node2;
    dtree_node_t * node3;
    dtree_node_t * retval;

    node1 = dtree_create_node(&dtree_root, "usr", 0);
    node2 = dtree_create_node(node1, "ab", 0);

    retval = dtree_lookup("/usr/ab", DTREE_LOOKUP_MATCH_EXACT);
    pu_assert_ptr_equal("Got ab node", retval, node2);

    node3 = dtree_create_node(node1, "aab", 0);

    retval = dtree_lookup("/usr/ab", DTREE_LOOKUP_MATCH_ANY);
    pu_assert_ptr_equal("Lookup still returns ab node", retval, node2);

    retval = dtree_lookup("/usr/aab", DTREE_LOOKUP_MATCH_ANY);
    pu_assert_ptr_equal("Got usr node", retval, node3);

    return 0;
}

static void all_tests() {
    pu_def_test(test_path_compare, PU_RUN);
    pu_def_test(test_create, PU_RUN);
    pu_def_test(test_getpath, PU_RUN);
    pu_def_test(test_lookup, PU_RUN);
    pu_def_test(test_remove, PU_RUN);
    pu_def_test(test_dicard, PU_RUN);
    pu_def_test(test_collision, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

