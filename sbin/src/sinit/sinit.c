/*
 * MIT/X Consortium License
 *
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2015, 2016, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/elf_notes.h>
#include <sys/priv.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

ELFNOTE_CAPABILITIES(
    PRIV_TTY_SETA,
    PRIV_VFS_READ,
    PRIV_VFS_WRITE,
    PRIV_VFS_EXEC,
    PRIV_VFS_LOOKUP,
    PRIV_VFS_STAT,
    PRIV_SIGNAL_OTHER,
    PRIV_SIGNAL_ACTION,
);

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
        return EXIT_FAILURE;

    chdir("/");
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);

    spawn(rcinitcmd);

    while (1) {
        sigwait(&set, &sig);
        for (i = 0; i < num_elem(sigmap); i++) {
            if (sigmap[i].sig == sig) {
                sigmap[i].handler();
                break;
            }
        }
    }

    /* not reachable */
    return 0;
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
        perror("sinit:fork");
    } else if (pid == 0) {
        sigprocmask(SIG_UNBLOCK, &set, NULL);

        setsid();
        execvp(argv[0], argv);
        fprintf(stderr, "sinit:exec: %s %s\n", argv[0], strerror(errno));
        _exit(1);
    }
}
