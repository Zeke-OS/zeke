#include <stdio.h>
#include <eztrie.h>

void print_eztrie(char * str, struct eztrie_iterator it)
{
    struct eztrie_node_value * value;

    printf("List for \"%s\":\n", str);
    while ((value = eztrie_remove_ithead(&it))) {
        printf("key: %s, value: %d\n", value->key, *((int *)value->p));
    }
    printf("\n");
}

int main(void)
{
    struct eztrie trie;
    struct keyval {
        char key[10];
        int value;
    } keyval[] = {
        { .key = "able", .value = 1 },
        { .key = "about", .value = 2 },
        { .key = "above", .value = 3 },
        { .key = "accept", .value = 4 },
        { .key = "across", .value = 5 },
        { .key = "act", .value = 6 },
        { .key = "actually", .value = 7 },
        { .key = "add", .value = 8 },
        { .key = "admit", .value = 9 },
        { .key = "afraid", .value = 10 },
        { .key = "after", .value = 11 },
        { .key = "afternoon", .value = 12 },
    };
    char * search[] = { "able", "ab", "aft", "", "x", NULL };
    char ** str;

    trie = eztrie_create();

    for (int i = 0; i < sizeof(keyval) / sizeof(*keyval); i++) {
        eztrie_insert(&trie, keyval[i].key, &keyval[i].value);
    }
    eztrie_remove(&trie, "add");

    str = search;
    do {
        print_eztrie(*str, eztrie_find(&trie, *str));
    } while (*++str);

    eztrie_destroy(&trie);

    return 0;
}
