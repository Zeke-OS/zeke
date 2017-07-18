/**
 *******************************************************************************
 * @file    getty.c
 * @author  Olli Vanhoja
 * @brief   Single process multi-tty get teletype.
 * @section LICENSE
 * Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>

struct gettytab_entry {
    char tty_devname[SPECNAMELEN];
    int tty_bdrate;
    int tty_csize; /* 5, 6, 7, 8 bits */
    struct {
        int cstopb : 1; /* s */
        int parenb : 1; /* p */
        int parodd : 1; /* o */
    } tty_ctrl;
    char login[40];
    pid_t pid; /* pid of process handling this tty */
};

extern char ** environ;

static struct gettytab_entry * tty_arr;
static size_t nr_tty;

static char * get_next_line(FILE * fp)
{
    static char line[256];

    while (fgets(line, sizeof(line), fp)) {
        char * cp;
        int ch;

        cp = strchr(line, '\n');
        if (cp) {
            *cp = '\0';
            return line;
        }

        /* skip lines that are too long */
        while ((ch = fgetc(fp)) != '\n' && ch != EOF);
    }

    return NULL;
}

/**
 * Read next entry from gettytab.
 */
static int next_entry(FILE * fp, struct gettytab_entry * entry)
{
    char * line;

    while ((line = get_next_line(fp))) {
        char * cp;
        char * bp;

        if (*line == '#')
            continue;

        bp = line;
        cp = strsep(&bp, ":");
        strlcpy(entry->tty_devname, cp, sizeof(entry->tty_devname));
        cp = strsep(&bp, ":");
        if (!cp)
            continue;
        entry->tty_bdrate = atoi(cp);
        cp = strsep(&bp, ":");
        if (!cp)
            continue;
        /* 8 - x so we get CSIZE values. */
        entry->tty_csize = (8 - atoi(cp)) & CSIZE;
        cp = strsep(&bp, ":");
        entry->tty_ctrl.cstopb = (strstr(cp, "s")) ? 1 : 0;
        entry->tty_ctrl.parenb = (strstr(cp, "p")) ? 1 : 0;
        entry->tty_ctrl.parodd = (strstr(cp, "o")) ? 1 : 0;
        cp = strsep(&bp, ":");
        strlcpy(entry->login, cp, sizeof(entry->login));

        entry->pid = -1;

        return 1;
    }

    return 0;
}

/**
 * Read gettytab to memory.
 */
static size_t read_gettytab(void)
{
    FILE * fp;
    struct gettytab_entry entry;
    struct gettytab_entry * tty = NULL;
    size_t i = 0;

    fp = fopen("/etc/gettytab", "r");
    if (!fp)
        return 0;

    while (next_entry(fp, &entry)) {
        tty_arr = realloc(tty, (i + 1) * sizeof(struct gettytab_entry));
        if (!tty_arr) {
            i = 0;
            goto fail;
        }

        tty_arr[i++] = entry;
    }

fail:
    fclose(fp);

    nr_tty = i;
    return i;
}

/**
 * Open and configure a tty.
 */
static int open_tty(struct gettytab_entry * entry)
{
    char filename[5 + SPECNAMELEN];
    int fd[3];

    sprintf(filename, "/dev/%s", entry->tty_devname);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    setsid();

    fd[0] = open(filename, O_RDONLY);
    fd[1] = open(filename, O_WRONLY);
    fd[2] = open(filename, O_WRONLY);

    for (size_t i = 0; i < 3; i++) {
        struct termios termios;

        if (fd[i] < 0)
            return 1;

        tcgetattr(fd[i], &termios);
        cfsetispeed(&termios, entry->tty_bdrate);
        cfsetospeed(&termios, entry->tty_bdrate);
        termios.c_cflag = entry->tty_csize |
                          (entry->tty_ctrl.cstopb) ? CSTOPB : 0 |
                          (entry->tty_ctrl.parenb) ? PARENB : 0 |
                          (entry->tty_ctrl.parodd) ? PARODD : 0;

        /* RFE Any optional actions needed? */
        if (tcsetattr(fd[i], 0, &termios)) {
            return 1;
        }
    }

    return 0;
}

/**
 * Setup a service on tty.
 */
static void setup_tty(struct gettytab_entry * tty)
{
        pid_t pid;

        pid = fork();
        if (pid == 0) {
            if (open_tty(tty))
                exit(EX_IOERR);

            execle(tty->login, basename(tty->login), NULL, environ);
            fprintf(stderr, "getty: exec %s failed\n", tty->login);
            exit(EX_OSERR);
        } else if (pid > 0) {
            tty->pid = pid;
        } else {
            perror("getty: Failed to fork");
        }
}

/**
 * Respawn a dead tty service.
 */
static void respawn_tty(void)
{
    pid_t pid;
    size_t i;

    pid = wait(NULL);

    for (i = 0; i < nr_tty; i++) {
        if (tty_arr[i].pid == pid) {
            setup_tty(&tty_arr[i]);
        }
    }
}

/**
 * Reload configuration for all ttys.
 */
static void reload_gettytab(void)
{
    size_t i;

    if (tty_arr)
        free(tty_arr);

    if (read_gettytab() == 0)
        exit(EX_OSERR);

    for (i = 0; i < nr_tty; i++) {
        setup_tty(&tty_arr[i]);
    }
}

int main(int argc, char * argv[])
{
    sigset_t sigset;
    struct {
        int sig;
        void (*handler)(void);
    } sigmap[] = {
        { SIGCHLD, respawn_tty },
        { SIGHUP, reload_gettytab },
    };

    sigfillset(&sigset);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    reload_gettytab();

    while (1) {
        int sig;
        size_t i;

        sigwait(&sigset, &sig);
        for (i = 0; i < num_elem(sigmap); i++) {
            if (sigmap[i].sig == sig) {
                sigmap[i].handler();
                break;
            }
        }
    }

    return EX_OSERR;
}
