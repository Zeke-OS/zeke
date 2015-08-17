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

static char * test_open_pty(void)
{
    masterfd = posix_openpt(O_RDWR|O_NOCTTY);
    pu_assert("master tty opened", masterfd > 0);

    pu_assert("grant", grantpt(masterfd) == 0);
    pu_assert("unlockpt()", unlockpt(masterfd) == 0);

    slavedev = ptsname(masterfd);
    pu_assert("get ptsname", slavedev != NULL);

    slavefd = open(slavedev, O_RDWR|O_NOCTTY);
    pu_assert("slave tty opened", slavefd > 0);

    return NULL;
}

static void all_tests()
{
    pu_run_test(test_open_pty);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
