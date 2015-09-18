#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "punit.h"

int masterfd, slavefd;
char *slavedev;

static void setup()
{
    masterfd = 0;
    slavefd = 0;
    slavedev = NULL;
}

static void teardown()
{
#if 0
    printf("errno: %d, masterfd: %d\n", errno, masterfd);
#endif

    if (slavefd >= 0)
        close(slavefd);

    if (masterfd >= 0)
        close(masterfd);
}

static char * open_pty(void)
{
    masterfd = posix_openpt(O_RDWR | O_NOCTTY);
    pu_assert("master tty opened", masterfd > 0);

    pu_assert("grant", grantpt(masterfd) == 0);
    pu_assert("unlockpt()", unlockpt(masterfd) == 0);

    slavedev = ptsname(masterfd);
    pu_assert("get ptsname", slavedev != NULL);

    slavefd = open(slavedev, O_RDWR | O_NOCTTY);
    pu_assert("slave tty opened", slavefd > 0);

    return NULL;
}

static char * test_open_pty(void)
{
    return open_pty();
}

static char * test_master2slave(void)
{
    char wr[] = "test";
    char rd[sizeof(wr)];
    char * res;
    ssize_t retval;

    res = open_pty();
    if (res)
        return res;

    retval = write(masterfd, wr, sizeof(wr));
    pu_assert_equal("write to master ok", retval, sizeof(wr));

    retval = read(slavefd, rd, sizeof(wr));
    pu_assert_equal("read from slave ok", retval, sizeof(wr));

    return NULL;
}

static char * test_slave2master(void)
{
    char wr[] = "test";
    char rd[sizeof(wr)];
    char * res;
    ssize_t retval;

    res = open_pty();
    if (res)
        return res;

    retval = write(slavefd, wr, sizeof(wr));
    pu_assert_equal("write to slave ok", retval, sizeof(wr));

    retval = read(masterfd, rd, sizeof(wr));
    pu_assert_equal("read from master ok", retval, sizeof(wr));

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_open_pty, PU_RUN);
    pu_def_test(test_master2slave, PU_RUN);
    pu_def_test(test_slave2master, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
