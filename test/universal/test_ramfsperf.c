/**
 * @file test_ramfs_perf.c
 * @brief Test ramfs performance.
 */

#include_next <time.h>
#include <sys/time.h>
#include <punit.h>
#include <kmalloc.h>
#include "sim_kmheap.h"
#include <fs/fs.h>

#define GET_TIME(tvar, tv)  (gettimeofday(&tv, NULL), tvar = tv.tv_sec * 1000 + tv.tv_usec / 1000)
#define PRINT_SUMMARY(start, end, bytes, mdl_text) \
    (printf("\t%u bytes %s, %u ms, %.2f MB/s\n", (bytes), mdl_text, end - start, \
    (((float)bytes/1024/1024*1000)/(float)(end-start))))

extern fs_t ramfs_fs;


static void setup()
{
    setup_kmalloc();
    ramfs_fs.sbl_head = 0;
}

static void teardown()
{
    teardown_kmalloc();
}

static char * perftest_wr_rd_reg_single(void)
{
#define MOUNT_POINT "/tmp"
#define MODE_FLAGS 0
#define FILENAME "test"
#define TEST_LEN (100 * 1024 * 1024)
    fs_superblock_t * sb;
    vnode_t * root;
    vnode_t * file;
    char * str_src;
    char * str_src1;
    char * str_dst;
    size_t bytes_written, bytes_read;
    const off_t file_start = 0;

    pu_test_description("Test write & read performance with one long block.");

    /* Get some "random" data */
    str_src = kmalloc(TEST_LEN);
    pu_assert("", str_src != 0);
    str_src1 = kmalloc(TEST_LEN);
    pu_assert("", str_src1 != 0);
    str_dst = kmalloc(TEST_LEN);
    pu_assert("", str_dst != 0);

    /* TODO After inode & super block removal is implemented this test could be
     * looped some times and then calculate an average of those runs. */

    sb = ramfs_fs.mount(MOUNT_POINT, MODE_FLAGS, 0, "");
    root = sb->root;
    pu_assert("Root exist", root != 0);

    root->vnode_ops->create(root, FILENAME, sizeof(FILENAME) - 1, &file);
    pu_assert("File was created.", file != 0);

    printf("Performance test:\n");
    {
        struct timeval tv;
        uint32_t start, end;

        /* Writing to the file */
        printf("- Write to a new file:\n");
        GET_TIME(start, tv);
        bytes_written = file->vnode_ops->write(file, &file_start, str_src, TEST_LEN);
        pu_assert_equal("Bytes written equals length of given buffer.",
                (int)bytes_written, (int)TEST_LEN);
        GET_TIME(end, tv);
        PRINT_SUMMARY(start, end, bytes_written, "written");

        printf("- Write to an existing file:\n");
        GET_TIME(start, tv);
        bytes_written = file->vnode_ops->write(file, &file_start, str_src1, TEST_LEN);
        pu_assert_equal("Bytes written equals length of given buffer.",
                (int)bytes_written, (int)TEST_LEN);
        GET_TIME(end, tv);
        PRINT_SUMMARY(start, end, bytes_written, "written");

        /* Reading from the file */
        printf("- Read file:\n");
        GET_TIME(start, tv);
        bytes_read = file->vnode_ops->read(file, &file_start, str_dst, TEST_LEN);
        pu_assert_equal("Bytes read equals length of the original buffer.",
                (int)bytes_read, (int)TEST_LEN);
        GET_TIME(end, tv);
        PRINT_SUMMARY(start, end, bytes_written, "read");
    }

    pu_assert_str_equal("String read from the file equal the original string.",
            str_dst, str_src1);

    kfree(str_src);
    kfree(str_src1);
    kfree(str_dst);

#undef MOUNT_POINT
#undef MODE_FLAGS
#undef FILENAME
    return 0;
}

static char * perftest_wr_rd_reg_multi(void)
{
#define MOUNT_POINT "/tmp"
#define MODE_FLAGS 0
#define FILENAME "test"
#define str_src "QAZWSXEDCEDCRFV"
#define str_src1 "JrewprggkwreREG"
#define BLOCKS 5000000
    fs_superblock_t * sb;
    vnode_t * root;
    vnode_t * file;
    char str_dst[sizeof(str_src) + 1];
    size_t bytes_written, bytes_read;
    const off_t file_start = 0;
    off_t offset;
    size_t i;
    struct timeval tv;
    uint32_t start, end;

    str_dst[sizeof(str_src)] = '\0';

    pu_test_description("Test write & read performance with short buffer blocks (sequential writes).");

    sb = ramfs_fs.mount(MOUNT_POINT, MODE_FLAGS, 0, "");
    root = sb->root;
    pu_assert("Root exist", root != 0);

    root->vnode_ops->create(root, FILENAME, sizeof(FILENAME) - 1, &file);
    pu_assert("File was created.", file != 0);

    printf("Performance test:\n");

    /* Writing to the file */
    printf("- Write to a new file:\n");
    GET_TIME(start, tv);
    for (offset = file_start, i = 0; i < BLOCKS; i++) {
        bytes_written = file->vnode_ops->write(file, &offset, str_src,
                sizeof(str_src));
        pu_assert_equal("Bytes written equals length of given buffer.",
                (int)bytes_written, (int)sizeof(str_src));
        offset += bytes_written;
    }
    GET_TIME(end, tv);
    PRINT_SUMMARY(start, end, offset, "written");

    printf("- Write to an existing file:\n");
    GET_TIME(start, tv);
    for (offset = file_start, i = 0; i < BLOCKS; i++) {
        bytes_written = file->vnode_ops->write(file, &offset, str_src1,
                sizeof(str_src1));
        pu_assert_equal("Bytes written equals length of given buffer.",
                (int)bytes_written, (int)sizeof(str_src1));
        offset += bytes_written;
    }
    GET_TIME(end, tv);
    PRINT_SUMMARY(start, end, offset, "written");

    /* Reading from the file */
    printf("- Read file:\n");
    GET_TIME(start, tv);
    for (offset = file_start, i = 0; i < BLOCKS; i++) {
        bytes_read = file->vnode_ops->read(file, &offset, str_dst, sizeof(str_src));
        pu_assert_equal("Bytes read equals length of the original buffer.",
                (int)bytes_read, (int)sizeof(str_src));
        offset += bytes_read;
    }
    GET_TIME(end, tv);
    PRINT_SUMMARY(start, end, (size_t)offset, "read");

#undef MOUNT_POINT
#undef MODE_FLAGS
#undef FILENAME
#undef str_src
    return 0;
}

static void all_tests() {
    pu_def_test(perftest_wr_rd_reg_single, PU_RUN);
    pu_def_test(perftest_wr_rd_reg_multi, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

