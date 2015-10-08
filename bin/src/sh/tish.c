/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny shell.
 * @section LICENSE
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <autoconf.h> /* for KERNEL_VERSION */
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <linenoise.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../utils.h"
#include "tish.h"

/* TODO should be without prefix but FAT doesn't support it */
#define HISTFILENAME "tish.histfile"

static char histfilepath[256];
static char * argv0;

void init_hist(void)
{
    char * histsize;
    char * home;
    int len;

    histsize = getenv("HISTSIZE");
    if (histsize) {
        len = atoi(histsize);
    } else {
        len = 1000;
    }
    linenoiseHistorySetMaxLen(len);

    home = getenv("HOME");
    if (home) {
        if (strlcpy(histfilepath, home, sizeof(histfilepath)) >
            (sizeof(histfilepath) - sizeof(HISTFILENAME) - 1)) {
            fprintf(stderr, "%s: Failed to get histfile path\n", argv0);
            exit(EXIT_FAILURE);
        }
    } else {
        strcpy(histfilepath, "/");
    }
    len = strlen(histfilepath) - 1;
    if (histfilepath[len - 1] != '/')
        histfilepath[len] = '/';
    strcpy(histfilepath + len + 1, HISTFILENAME);

    linenoiseHistoryLoad(histfilepath);
}


static char * get_prompt(void)
{
    char * ps1;
    size_t i = 0;
    static char prompt[40];
    char buf[40];

    ps1 = getenv("PS1");
    if (!ps1) {
        ps1 = "# ";
        return ps1;
    }

    while (*ps1 != '\0') {
        if (i >= sizeof(prompt) - 1)
            break;

        if (*ps1 != '\\') {
            prompt[i++] = *ps1++;
            continue;
        }
        if (*++ps1 == '\0')
            break;
        switch (*ps1) {
        case 'a': /* Bell */
            prompt[i++] = '\a';
            break;
        case 'd': /* TODO Date eg. "Tue May 26" */
            break;
        case 'e':
            prompt[i++] = '\033';
            break;
        case 'h':
        case 'H':
            if (!gethostname(buf, sizeof(buf))) {
                i += strlcpy(prompt + i, buf, sizeof(prompt) - i);
            }
            break;
        case 'n':
            prompt[i++] = '\n';
            break;
        case 'r':
            prompt[i++] = '\r';
            break;
        case 's':
            i += strlcpy(prompt + i, argv0, sizeof(prompt) - i);
            break;
        /* TODO t, T, and @ */
        case 't':
            break;
        case 'T':
            break;
        case '@':
            break;
        case 'u': /* TODO username of the current user */
            break;
        case 'w':
        case 'W': {
                char * pwd;
                char * pwd2;

                pwd = getcwd(buf, sizeof(buf));
                if (!pwd)
                    break;
                if (*ps1 == 'W' && (pwd2 = basename(pwd)))
                    pwd = pwd2;
                if (pwd)
                    i += strlcpy(prompt + i, pwd, sizeof(prompt) - i);
                break;
            }
        case '$':
            prompt[i++] = (getuid() == 0) ? '#' : '$';
            break;
        case '\\':
            prompt[i++] = '\\';
            break;
        }
        ps1++;
    }
    prompt[sizeof(prompt) - 1] = '\0';

    return prompt;
}

int main(int argc, char * argv[], char * envp[])
{
    char * line;

    argv0 = argv[0];

    /* TODO get opts */
    argv++;

    if (argc == 2) {
        /* File mode */
        int fd;
        FILE * fp;

        if ((fd = open(argv[0], O_EXEC | O_RDONLY | O_CLOEXEC)) == -1 ||
            !(fp = fdopen(fd, "r")) ||
            !(line = malloc(MAX_INPUT))) {
            exit(EXIT_FAILURE);
        }

        while (fgets(line, MAX_INPUT, fp)) {
            run_line(line);
        }

        fclose(fp);
    } else {
        init_hist();

        fflush(NULL);
        while ((line = linenoise(get_prompt())) != NULL) {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(histfilepath);

            run_line(line);
            free(line);
        }

        perror("Failed to read stdin");
    }

    return 0;
}

static int help(char * argv[])
{
    struct tish_builtin ** cmd;

    SET_FOREACH(cmd, tish_cmd) {
        printf("%s ", (*cmd)->name);
    }

    printf("\n");

    return 0;
}
TISH_CMD(help, "help");
