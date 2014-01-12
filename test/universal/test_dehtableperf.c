/**
 * @file test_dehtableperf.c
 * @brief Test directory entry hash table performance.
 */

#include <punit.h>
#include <stdlib.h>
#include_next <time.h>
#include <sys/time.h>
#include "sim_heap.h"
#include <kmalloc.h>
#include <fs/fs.h>
#include <fs/dehtable.h>

#define get_dirent(dea, offset) ((dh_dirent_t *)((size_t)(dea) + (offset)))

#define GET_TIME(tvar, tv)      (gettimeofday(&tv, NULL), tvar = tv.tv_sec * 1000 + tv.tv_usec / 1000)
#define GET_TIME_US(tvar, tv)   (gettimeofday(&tv, NULL), tvar = tv.tv_sec * 1000000 + tv.tv_usec)

static char * rand_string(char *str, size_t size);
static int unirand(int n);

dh_table_t table;

static void setup()
{
    setup_kmalloc();
    memset(table, 0, sizeof(dh_table_t));
}

static void teardown()
{
    size_t i;

    /* Free chain arrays */
    for (i = 0; i < DEHTABLE_SIZE; i++) {
        if (table[i] != 0)
            kfree(table[i]);
    }
    teardown_kmalloc();
}

static char * test_link_perf(void)
{
#define MAX     20000
#define POINTS  20
    vnode_t vnode;
    char str[4];
    dh_dirent_t * dea;
    size_t i, j;
    const size_t d = MAX / POINTS;
    struct timeval tv;
    uint32_t start, end;

    pu_test_description("Test dh_link performance.");

    printf("Links\tTime (ms)\n");
    for (i = d; i <= MAX; i += d) {
        GET_TIME(start, tv);
        for (j = 0; j < i; j++) {
            vnode.vnode_num = unirand(10000); /* Get random vnode number */
            rand_string(str, sizeof(str));
            dh_link(&table, &vnode, str, sizeof(str) - 1);
        }
        GET_TIME(end, tv);
        printf("%u\t%u\n", i, end - start);
    }

#undef POINTS
#undef MAX

    return 0;
}

static char * test_lookup_perf(void)
{
#define MIN     2000
#define MAX     20000
#define POINTS  20
#define MEAN    100 /* Mean of n lookups */
    vnode_t vnode;
    char str[4];
    dh_dirent_t * dea;
    size_t i, j, k;
    const size_t d = MAX / POINTS;
    struct timeval tv;
    uint64_t start, end;
    uint64_t mean;
    ino_t nnum;
    unsigned int ok;

    pu_test_description("Test dh_lookup performance.");

    printf("Links\tt_mean (us)\t%% found\n");
    for (i = MIN; i <= MAX; i += d) {
        for (j = 0; j < i; j++) {
            vnode.vnode_num = unirand(10000); /* Get random vnode number */
            rand_string(str, sizeof(str));
            dh_link(&table, &vnode, str, sizeof(str) - 1);
        }

        ok = 0;
        mean = 0;
        for (k = 0; k < MEAN; k++) {
            GET_TIME_US(start, tv);
            rand_string(str, sizeof(str));
            ok += !dh_lookup(&table, str, sizeof(str) - 1, &nnum) ? 1 : 0;
            GET_TIME_US(end, tv);
            mean += end - start;
        }
        mean /= MEAN;
        printf("%u\t%u\t\t%f\n", i, mean, (float)ok/(float)MEAN);
    }

#undef MEAN
#undef POINTS
#undef MAX

    return 0;
}

static void all_tests() {
    pu_def_test(test_link_perf, PU_RUN);
    pu_def_test(test_lookup_perf, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = unirand((int)(sizeof charset - 1));
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

static int unirand(int n)
{
    int partSize = (n == RAND_MAX) ? 1 : 1 + (RAND_MAX - n)/(n + 1);
    int maxUsefull = partSize * n + (partSize - 1);
    int draw;
    do {
        draw = rand();
    } while (draw > maxUsefull);

    return draw/partSize;
}
