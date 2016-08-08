/**
 *******************************************************************************
 * @file    popen.c
 * @author  Olli Vanhoja
 * @brief   Standard functions.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <sys/_PDCLIB_io.h>
#include <unistd.h>

#define READ    0
#define WRITE   1

FILE *popen(const char * command, const char * mode)
{
    int pipettes[2];
    pid_t pid;

    if (mode[0] != 'r' && mode[0] != 'w') {
        errno = EINVAL;
        return NULL;
    }

    pipe(pipettes);
    pid = fork();
    if (pid == -1) {
        close(pipettes[READ]);
        close(pipettes[WRITE]);
        return NULL;
    } else if (pid == 0) {
        if (mode[0] == 'r') {
            dup2(pipettes[WRITE], STDOUT_FILENO);
            close(pipettes[READ]);
        } else if (mode[0] == 'w') {
            dup2(pipettes[READ], STDIN_FILENO);
            close(pipettes[WRITE]);
        }
        execl(_PATH_BSHELL, "sh", "-c", command, (char *)0);
        return NULL;
    } else {
        FILE * fp = NULL;

        if (mode[0] == 'r') {
            close(pipettes[WRITE]);
            fp = fdopen(pipettes[READ], "r");
            fp->pid = pid;
        } else if (mode[0] == 'w') {
            close(pipettes[READ]);
            fp = fdopen(pipettes[WRITE], "w");
            fp->pid = pid;
        }

        return fp;
    }
}
