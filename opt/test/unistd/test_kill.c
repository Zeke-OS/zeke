#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zeke.h>
#include "punit.h"

char * thread_stack;
pthread_t thread_id;
int thread_unslept;
uintptr_t sp_before, sp_after;
int thread_signum_received[2];

static void catch_sig(int signum)
{
    /*printf("sighandler called, sig: %i\n", signum);*/
    thread_signum_received[0] = signum;
}

static void * thread(void * arg)
{
    sigset_t waitset;
    int signum;

    signal(SIGUSR1, catch_sig);

    __asm__ volatile (
        "mov %[reg], sp\n\t"
        : [reg]"=r" (sp_before));

    fprintf(stderr, ".");
    thread_unslept = sleep(10);

    __asm__ volatile (
        "mov %[reg], sp\n\t"
        : [reg]"=r" (sp_after));

    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &waitset, NULL);
    sigwait(&waitset, &signum);
    thread_signum_received[1] = signum;


    return NULL;
}

static void setup(void)
{
    pthread_attr_t attr;
    const size_t stack_size = 4096;

    thread_signum_received[0] = 0;
    thread_signum_received[1] = 0;

    thread_stack = malloc(stack_size);
    if (!thread_stack) {
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
    sleep(1);
}

static void teardown(void)
{
    int * retval;

    pthread_cancel(thread_id);
    pthread_join(thread_id, (void **)(&retval));
    free(thread_stack);
}

static char * test_kill_thread(void)
{
    fprintf(stderr, ".");
    sleep(2);
    fprintf(stderr, ".");
    pthread_kill(thread_id, SIGUSR1);
    sleep(1);
    fprintf(stderr, ".");
    pu_assert_equal("", thread_signum_received[0], SIGUSR1);
    pu_assert("Sleep was interrupted", thread_unslept > 0);
    pu_assert_equal("sp was preserved properly", sp_before, sp_after);

    sleep(1);
    fprintf(stderr, ".");
    pthread_kill(thread_id, SIGUSR2);
    sleep(1);
    fprintf(stderr, ".");
    pu_assert_equal("", thread_signum_received[1], SIGUSR2);

    return NULL;
}

static void all_tests(void)
{
    pu_def_test(test_kill_thread, PU_RUN);
}

int main(int argc, char ** argv)
{
    return pu_run_tests(&all_tests);
}
