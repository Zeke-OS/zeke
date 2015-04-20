#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include "punit.h"

char * thread_stack;
pthread_t thread_id;
int thread_signum_received[2];

static void catch_sig(int signum)
{
    thread_signum_received[0] = signum;
}

static void * thread(void * arg)
{
    sigset_t waitset;
    int sig;

    signal(SIGUSR1, catch_sig);

    sigaddset(&waitset, SIGUSR2);
    sigwait(waitset, &sig);
    thread_signum_received[1] = sig;

    thread_id = 0;
    return &thread_signum_received[1];
}

static void setup()
{
    pthread_attr_t attr;
    const size_t stack_size = 4096;

    thread_stack = malloc(stack_size);
    if (!stack) {
        perror("Failed to create a stack\n");
        _exit(1);
    }

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr, thread_stack, stack_size);

    errno = 0;
    if (pthread_create(&thread_id, &attr, thread, 0)) {
        perror("Thread creation failed\n");
        _exit(1);
    }
}

static void teardown()
{
    int retval;

    if (thread_id) {
        pthread_kill(thread_id, SIGKILL);
        pthread_join(thread_id, &retval);
    }
    free(stack);
}

static char * test_kill_thread(void)
{
    /* TODO Test USR1 (handler) and USR1 (sigwait) */

    return NULL;
}

static void all_tests()
{
    pu_def_test(test_kill_thread, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
