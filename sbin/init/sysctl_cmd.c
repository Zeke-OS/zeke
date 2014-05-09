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
#include <unistd.h>
#include <libkern.h>
#include <sys/sysctl.h>
#include <errno.h>
#include "tish.h"

static void list_all(void);
static void print_mib_name(int * mib, int len);

/* TODO Remove */
#define fprintf(stream, str) write(2, str, strlenn(str, MAX_LEN))
#define puts(str) fprintf(stderr, str)

void sysctl_cmd(char ** args)
{
    char * arg = kstrtok(0, DELIMS, args);

    if (!strcmp(arg, "-a")) {
        list_all();
    }
}

static void list_all(void)
{
    int mib[CTL_MAXNAME] = {0};
    size_t len = 0, len_next;
    int  err;

    while ((len_next = sizeof(mib)), !(err = sysctlgetnext(mib, len, mib, &len_next))) {
        len = len_next;
        print_mib_name(mib, len);
    }
}

static void print_mib_name(int * mib, int len)
{
    char buf[41];
    char strname[40];
    size_t strname_len = sizeof(strname);

    sysctlmibtoname(mib, len, strname, &strname_len);
    ksprintf(buf, sizeof(buf), "%s\n", strname);
    puts(buf);
}
