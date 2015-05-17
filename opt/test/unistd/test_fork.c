#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "punit.h"

pid_t pid;

static void setup(void)
{
}

static void teardown(void)
{
    if (pid > 0)
        kill(pid, SIGKILL);
}

static char * test_fork_created(void)
{
    pid = fork();
    pu_assert("Fork created", pid != -1);

    if (pid == 0) {
        exit(0);
    } else {
        wait(NULL);
        pid = 0;
    }

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_fork_created, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
