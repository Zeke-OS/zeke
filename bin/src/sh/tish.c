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
#include <linenoise.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../utils.h"
#include "tish.h"

/* TODO should be without prefix but FAT doesn't support ot */
#define HISTFILENAME "tish.histfile"

char banner[] = "\
|'''''||                    \n\
    .|'   ...'||            \n\
   ||   .|...|||  ..  ....  \n\
 .|'    ||    || .' .|...|| \n\
||......|'|...||'|. ||      \n\
             .||. ||.'|...'\n\n\
";
static const char msg[] = "Zeke " KERNEL_VERSION;
static char histfilepath[256];

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

int main(int argc, char * argv[], char * envp[])
{
    const char prompt[] = "# ";
    char * line;

    /* TODO get opts */
    argv += 1;

    if (argc == 2) {
        /* File mode */
        int fd;
        FILE * fp;

        printf("File parsing mode\n");
        fd = open(argv[0], O_EXEC | O_RDONLY | O_CLOEXEC);
        if (fd == -1) {
            exit(EXIT_FAILURE);
        }
        fp = fdopen(fd, "r");
        if (!fp) {
            exit(EXIT_FAILURE);
        }
        line = malloc(MAX_INPUT);
        if (!line) {
            exit(EXIT_FAILURE);
        }

        while (fgets(line, MAX_INPUT, fp)) {
            run_line(line);
        }
    } else {
        /* Interactive mode */
        printf("\n%s%s\n", banner, msg);

        init_hist();

        fflush(NULL);
        while ((line = linenoise(prompt)) != NULL) {
            linenoiseHistoryAdd(line);
            linenoiseHistorySave(histfilepath);

            run_line(line);
            free(line);
        }
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
