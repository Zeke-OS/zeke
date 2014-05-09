/**
 *******************************************************************************
 * @file    tish.c
 * @author  Olli Vanhoja
 * @brief   Tiny Init Shell for debugging in init.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <sys/types.h>
#include <kstring.h>
#include <libkern.h>
#include <errno.h>
#include <unistd.h>

#define MAX_LEN 80
#define DELIMS " \t\r\n"

#define fprintf(stream, str) write(2, str, strlenn(str, MAX_LEN))
#define puts(str) fprintf(stderr, str)

struct builtin {
    void (*fn)(char ** args);
    char name[10];
};

static void cd(char ** args);
static char * gline(char * str, int num);

struct builtin cmdarr[] = {
        {cd, "cd"}
};

int tish(void)
{
    char * cmd;
    char line[MAX_LEN];
    char * strtok_lasts;
    int err;

    while (1) {
prompt:
        puts("# ");
        if (!gline(line, MAX_LEN))
            break;

        if ((cmd = kstrtok(line, DELIMS, &strtok_lasts))) {
            /* TODO Clear errno */

            for (int i = 0; i < num_elem(cmdarr); i++) {
                if (!strcmp(cmd, cmdarr[i].name)) {
                    cmdarr[i].fn(&strtok_lasts);
                    goto prompt;
                }
            }

            puts("I don't know how to execute\n");

            if ((err = errno)) {
                /*perror("Failed");*/
                ksprintf(line, sizeof(line), "\nerrno: %u\n", err);
                puts(line);
            }
        }
    }

    return 0;
}

static void cd(char ** args)
{
    char * arg = kstrtok(0, DELIMS, args);
    puts("cd not implemented\n");
#if 0
    if (!arg)
        fprintf(stderr, "cd missing argument.\n");
    else
        chdir(arg);
#endif
}

/* TODO Until we have some actual standard IO subsystem working the following
 * is the best hack I can think of. */
#define _ugetc() bcm2835_ugetc()
#define _uputc(c) bcm2835_uputc(c)

static char * gline(char * str, int num)
{
    int c;
    int i = 0;

    while (1) {
        c = _ugetc();
        if (c < 0) {
            continue;
        }

        /* Handle backspace */
        if (c == 127) {
            if (i > 0) {
                i--;
                _uputc('\b');
                _uputc(' ');
                _uputc('\b');
            }
            continue;
        }

        /* TODO Handle arrow keys and delete */

        /* Handle return */
        if (c == '\n' || c == '\r' || i == num) {
            str[i] = '\0';
            _uputc('\n');
            return str;
        } else {
            str[i] = (char)c;
        }

        _uputc(c);
        i++;
    }
}
