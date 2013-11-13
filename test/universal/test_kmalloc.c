/**
 * @file test_bitmap.c
 * @brief Test bitmap functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <time.h>
#include <punit.h>
#include <kmalloc.h>

typedef struct mblock {
    size_t size;
    struct mblock * next;
    struct mblock * prev;
    int refcount;       /*!< Ref count. */
    void * ptr;         /*!< Sanity check pointer. */
    char data[1];
} mblock_t;
#define MBLOCK_SIZE (sizeof(mblock_t) - 4)
extern void * kmalloc_base;

static void rnd_allocs(int n);
static int unirand(int n);

typedef struct {
    char a[1024];
    char b[1024];
    char c[1024];
} simheap_t;
simheap_t simheap;

#define block(x, offset) ((mblock_t *)(((size_t)simheap.x) + offset))

static void setup()
{
    mblock_t * ma = (mblock_t *)(simheap.a);
    mblock_t * mb = (mblock_t *)(simheap.b);
    mblock_t * mc = (mblock_t *)(simheap.c);

    ma->size = sizeof(simheap.a) - MBLOCK_SIZE;
    ma->prev = 0;
    ma->next = 0;
    ma->ptr = ma->data;
    ma->refcount = 0;

    mb->size = sizeof(simheap.b) - MBLOCK_SIZE;
    mb->prev = 0;
    mb->next = 0;
    mb->ptr = mb->data;
    mb->refcount = 0;

    mb->size = sizeof(simheap.c) - MBLOCK_SIZE;
    mb->prev = 0;
    mb->next = 0;
    mb->ptr = mc->data;
    mb->refcount = 0;

    kmalloc_base = (void *)(simheap.a);
}

static void teardown()
{
}

static char * kmalloc_simple(void)
{
    void * p = kmalloc(100);

    pu_assert_not_null("No error on allocation", p);
    pu_assert_ptr_equal("Allocated 100 bytes just after the first descpriptor", p, block(a, 0)->data);

    return 0;
}
/*
static char * perf_test(void)
{
    const int trials = 3;
    int i, n;
    int result = 0;

    srand(time(NULL));

    printf("Performance test:\n");
    for (n = 1; n <= 4; n++) {
        result = 0;
        for (i = 0; i < trials; i++) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            uint32_t start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            rnd_allocs(n);

            gettimeofday(&tv, NULL);
            uint32_t end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            result += end - start;
        }
        printf("\tn = %i: %i ms\n", n, result / trials);
    }

    return 0;
}

static void rnd_allocs(int n)
{
    const int allocs = 8000;
    const int maxsim = 1000;
    int i, j, err;
    size_t ret, sti = 0;
    size_t st[maxsim][2];
    bitmap_t bmap[n][E2BITMAP_SIZE(2048)];

    memset(bmap, 0, sizeof(bmap));

    for (i = 0; i < allocs; i++) {
        for (j = 0; j < n; j++) {
            if (unirand(1)) {
                while (!(st[sti][1] = unirand(100)));
                err = bitmap_block_alloc(&ret, st[sti][1], bmap[j], sizeof(bmap[j]));
                if (err == 0) {
                    st[sti][0] = ret;
                    sti++;
                } else if (sti > 0) {
                    sti--;
                    bitmap_block_update(bmap[j], 0, st[sti][0], st[sti][1]);
                }
            }

            if ((unirand(1) || sti >= maxsim) && (sti > 0)) {
                sti--;
                bitmap_block_update(bmap[j], 0, st[sti][0], st[sti][1]);
            }
        }
    }
}
*/

static void all_tests() {
    pu_def_test(kmalloc_simple, PU_RUN);
    //pu_def_test(perf_test, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
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

