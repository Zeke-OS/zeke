/*
 * MIT/X Consortium License
 *
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2014 Dimitris Papastamos <sin@2f30.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>

/* Needed for tty handling */
#include <fcntl.h>

#define LEN(x) (sizeof(x) / sizeof(*(x)))

static void open_tty(void);
static void poweroff(void);
static void reap(void);
static void reboot(void);
static void spawn(char *const []);

static struct {
    int sig;
    void (*handler)(void);
} sigmap[] = {
    { SIGUSR1, poweroff },
    { SIGCHLD, reap     },
    { SIGINT,  reboot   },
};

#include "siconfig.h"

static sigset_t set;

int
main(void)
{
    int sig;
    size_t i;

    if (getpid() != 1)
        return 1;

    chdir("/");
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);

    open_tty();

    spawn(rcinitcmd);

    while (1) {
        sigwait(&set, &sig);
        for (i = 0; i < LEN(sigmap); i++) {
            if (sigmap[i].sig == sig) {
                sigmap[i].handler();
                break;
            }
        }
    }

    /* not reachable */
    return 0;
}

/* This is needed until we have better way to handle ttys */
static void open_tty(void)
{
    int r0, r1, r2;

    setenv("PATH", _PATH_STDPATH, 1);
    setenv("HOME", "/", 1);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    r0 = open(configSINIT_STDIN, O_RDONLY);
    r1 = open(configSINIT_STDOUT, O_WRONLY);
    r2 = open(configSINIT_STDERR, O_WRONLY);

    printf("fd: %i, %i, %i\n", r0, r1, r2);
}

static void
poweroff(void)
{
    spawn(rcpoweroffcmd);
}

static void
reap(void)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

static void
reboot(void)
{
    spawn(rcrebootcmd);
}

static void
spawn(char *const argv[])
{
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        sigprocmask(SIG_UNBLOCK, &set, NULL);

        /*
         * TODO We don't yet have setsid() available.
         * setsid();
         */
        execvp(argv[0], argv);
        perror("execvp");
        _exit(1);
    }
}
