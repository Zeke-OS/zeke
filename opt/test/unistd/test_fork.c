#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "punit.h"

pid_t pids[10];

static void setup(void)
{
    size_t i;

    for (i = 0; i < num_elem(pids); i++) {
        pids[i] = -1;
    }
}

static void teardown(void)
{
    size_t i;

    for (i = 0; i < num_elem(pids); i++) {
        pid_t pid = pids[i];

        if (pid > 0)
            kill(pid, SIGKILL);
    }
}

static char * test_fork_created(void)
{
    pids[0] = fork();
    pu_assert("Fork created", pids[0] != -1);

    if (pids[0] == 0) {
        exit(0);
    } else {
        int status;

        wait(&status);
        pu_assert("Child wasn't killed by a signal", WIFSIGNALED(status) == 0);
    }

    return NULL;
}

static char * test_fork_multi(void)
{
    size_t i;
    int status;
    pid_t self = getpid();

    for (i = 0; i < num_elem(pids); i++) {
        pid_t pid;

        pid = fork();
        pu_assert("Fork created", pid != -1);

        if (pid == 0) {
            exit(0);
        } else {
            pids[i] = pid;
        }
    }
    if (getpid() != self)
        exit(1);

    for (i = 0; i < num_elem(pids); i++) {
        size_t j;
        pid_t pid;

        pid = wait(&status);
        pu_assert("Child wasn't killed by a signal",
                  WIFSIGNALED(status) == 0);
        for (j = 0; j < num_elem(pids); j++) {
            if (pid == pids[j]) {
                pids[j] = -1;
                break;
            }
        }
    }

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_fork_created, PU_RUN);
    pu_def_test(test_fork_multi, PU_RUN);
}

int main(int argc, char **argv)
{
    return pu_run_tests(&all_tests);
}
