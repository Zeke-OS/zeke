/**
 * @file test_bitmap.c
 * @brief Test bitmap functions.
 */

#include <kunit/limits.h>
#if 0
#include <time.h>
#endif
#include <libkern.h>
#include <kunit.h>
#include <test/ktest_mib.h>
#include <generic/bitmap.h>

#if 0
static void rnd_allocs(int n);
#endif

static void setup()
{
}

static void teardown()
{
}

static char * test_search(void)
{
    bitmap_t bmap[32/4];
    size_t retval, err;

    memset(bmap, 0, sizeof(bmap));

    err = bitmap_block_search(&retval, 256, bmap, sizeof(bmap));
    ku_assert_equal("No error", err, 0);
    ku_assert_equal("retval ok", retval, 0);

    return 0;
}

static char * test_alloc(void)
{
    bitmap_t bmap[64];
    size_t ret, err;

    memset(bmap, 0, sizeof(bmap));

    err = bitmap_block_alloc(&ret, 4, bmap, sizeof(bmap));
    ku_assert_equal("No error on allocation", err, 0);
    ku_assert_equal("4 bits allocated from bitmap", bmap[0], 0xf);

    return 0;
}

#if 0
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
            if (kunirand(1)) {
                while (!(st[sti][1] = kunirand(100)));
                err = bitmap_block_alloc(&ret, st[sti][1], bmap[j], sizeof(bmap[j]));
                if (err == 0) {
                    st[sti][0] = ret;
                    sti++;
                } else if (sti > 0) {
                    sti--;
                    bitmap_block_update(bmap[j], 0, st[sti][0], st[sti][1]);
                }
            }

            if ((kunirand(1) || sti >= maxsim) && (sti > 0)) {
                sti--;
                bitmap_block_update(bmap[j], 0, st[sti][0], st[sti][1]);
            }
        }
    }
}
#endif

#if 0
static char * perf_test(void)
{
    const int trials = 3;
    int i, n;
    int result = 0;

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
#endif

static void all_tests() {
    ku_def_test(test_search, KU_RUN);
    ku_def_test(test_alloc, KU_RUN);
#if 0
    ku_def_test(perf_test, KU_RUN);
#endif
}

SYSCTL_TEST(generic, bitmap);
