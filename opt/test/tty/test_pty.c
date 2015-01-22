#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include "punit.h"

int masterfd, slavefd;
char *slavedev;

static void setup()
{
}

static void teardown()
{
}

static char * test_pty(void)
{
    masterfd = posix_openpt(O_RDWR|O_NOCTTY);

    pu_assert("master tty was opened", masterfd > 0);
    pu_assert("grant", grantpt(masterfd) == 0);

    slavedev = ptsname(masterfd);
    pu_assert("get ptsname", slavedev != NULL);
    printf("%s\n", slavedev);

    return NULL;
}

static void all_tests()
{
    pu_run_test(test_pty);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}

