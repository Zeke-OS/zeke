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
#include <syscall.h>
#include <sys/sysctl.h>
#include <errno.h>
#include "tish.h"

static void list_all(void);
static void getset_parm(char * arg);
static void getset_svalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len);
static void getset_ivalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len);
static void print_mib_name(int * mib, int len);

/* TODO Remove */
#define fprintf(stream, str) write(2, str, strlenn(str, MAX_LEN) + 1)
#define puts(str) fprintf(stderr, str)

void tish_sysctl_cmd(char ** args)
{
    char * arg = kstrtok(0, DELIMS, args);

    if (!strcmp(arg, "-a")) {
        list_all();
    } else {
        /* TODO do this correctly */
        /* We only support integer values by now */
        getset_parm(arg);
    }
}

static void getset_parm(char * arg)
{
    char * name;
    char * value;
    char * rest;

    int mib[CTL_MAXNAME];
    char fmt [5];
    unsigned int kind;
    int ctltype;
    size_t dlen;

    name = kstrtok(arg, "=", &rest);
    value = kstrtok(0, "=", &rest);
    if (!name) {
        puts("Invalid argument\n");
        return;
    }
    const size_t name_len = strlenn(name, 80);

    const int mib_len = sysctlnametomib(name, mib, num_elem(mib));
    if (mib_len < 0) {
        puts("Node not found\n");
        return;
    }

    {
        char buf[name_len + 5];
        ksprintf(buf, sizeof(buf), "%s = ", name);
        puts(buf);
    }

    if (sysctloidfmt(mib, mib_len, fmt, &kind)) {
        puts("Invalid node\n");
        return;
    }
    if (sysctl(mib, mib_len, 0, &dlen, 0, 0)) {
        puts("Invalid node\n");
        return;
    }

    ctltype = (kind & CTLTYPE);
    switch (ctltype) {
    case CTLTYPE_STRING:
        getset_svalue(mib, mib_len, dlen, value, strlenn(value, 80));
        break;
    case CTLTYPE_INT:
    case CTLTYPE_UINT:
        getset_ivalue(mib, mib_len, dlen, value, sizeof(int));
        break;
    case CTLTYPE_LONG:
    case CTLTYPE_ULONG:
    case CTLTYPE_S64:
    case CTLTYPE_U64:
        puts("Data type not supported yet\n");
        break;
    }
}

static void getset_svalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len)
{
    char ovalbuf[oval_len + 1];
    char buf[40 + oval_len];

    if(sysctl(oid, len, ovalbuf, &oval_len, (void *)nval, nval_len)) {
        ksprintf(buf, sizeof(buf), "Error: %u\n",
                (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
        puts(buf);
        return;
    }
    ksprintf(buf, sizeof(buf), "%s\n", ovalbuf);
    puts(buf);
}

static void getset_ivalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len)
{
    char buf[80];
    int x;
    size_t x_len = sizeof(x);

    /* Get value */
    if(sysctl(oid, len, &x, &x_len, 0, 0)) {
        ksprintf(buf, sizeof(buf), "Error: %u\n",
                (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
        puts(buf);
        return;
    }
    ksprintf(buf, sizeof(buf), "%u\n", x);
    puts(buf);

    /* Set value */
    if (nval) {
        x = atoi(nval);
        sysctl(oid, len, 0, 0, (void *)(&x), sizeof(int));
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

void tish_uname(char ** args)
{
    char * arg = kstrtok(0, DELIMS, args);
    int mib[2];
    int len;
    char str[20];
    char buf1[40];
    char buf2[40];
    size_t str_len = sizeof(str);

    len = sysctlnametomib("kern.ostype", mib, num_elem(mib));
    sysctl(mib, len, &str, &str_len, 0, 0);
    ksprintf(buf2, sizeof(buf2), "%s", str);

    if (!strcmp(arg, "-a")) {
        len = sysctlnametomib("kern.osrelease", mib, num_elem(mib));
        str_len = sizeof(str);
        sysctl(mib, len, &str, &str_len, 0, 0);
        ksprintf(buf1, sizeof(buf1), "%s %s", buf2, str);
        memcpy(buf2, buf1, sizeof(buf1));

        len = sysctlnametomib("kern.version", mib, num_elem(mib));
        str_len = sizeof(str);
        sysctl(mib, len, &str, &str_len, 0, 0);
        ksprintf(buf1, sizeof(buf1), "%s %s",buf2, str);
        memcpy(buf2, buf1, sizeof(buf1));
    }

    ksprintf(buf1, sizeof(buf1), "%s\n", buf2);
    puts(buf1);
}

void tish_ikut(char ** arg)
{
    char buf[80];
    int mib_test[5];
    int mib_cur[5];
    int mib_next[5];
    size_t len_cur, len_next;
    int  err;
    const int one = 1;

    const size_t len_test = sysctlnametomib("debug.test", mib_test, num_elem(mib_test));

    puts("     \n"); /* Hack */
    print_mib_name(mib_test, len_test);

    memcpy(mib_cur, mib_test, len_test * sizeof(int));
    len_cur = len_test;

    while ((len_next = sizeof(mib_next)),
            (err = sysctlgetnext(mib_cur, len_cur, mib_next, &len_next)) == 0) {
        if (!sysctltstmib(mib_next, mib_test, len_test)) {
            puts("End of tests\n");
            break; /* EOF debug.test */
        }
        memcpy(mib_cur, mib_next, len_next * sizeof(int));
        len_cur = len_next;

        print_mib_name(mib_cur, len_cur);
        sysctl(mib_cur, len_cur, 0, 0, (void *)(&one), sizeof(one));
    }

    ksprintf(buf, sizeof(buf), "errno = %i\n", errno);
    puts(buf);
}
