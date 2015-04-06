/**
 *******************************************************************************
 * @file    tish_sysctl.c
 * @author  Olli Vanhoja
 * @brief   Tiny Init Shell for debugging in init.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/sysctl.h>
#include <errno.h>
#include "tish.h"

static void list_all(void);
static int getset_parm(char * arg);
static void getset_svalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len);
static void getset_ivalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len);
static void print_mib_name(int * mib, int len);

static int tish_sysctl_cmd(char ** args)
{
    char * arg = strtok_r(0, DELIMS, args);
    int retval = 0;

    if (!strcmp(arg, "-a")) {
        list_all();
    } else {
        /* TODO do this correctly */
        /* We only support integer values by now */
        retval = getset_parm(arg);
    }

    return retval;
}
TISH_CMD(tish_sysctl_cmd, "sysctl");

static int getset_parm(char * arg)
{
    char * name;
    char * value;
    char * rest;

    int mib[CTL_MAXNAME];
    char fmt[5];
    unsigned int kind;
    int ctltype;
    size_t dlen;

    name = strtok_r(arg, "=", &rest);
    value = strtok_r(0, "=", &rest);
    if (!name) {
        printf("Invalid argument\n");

        errno = EINVAL;
        return -1;
    }

    const int mib_len = sysctlnametomib(name, mib, num_elem(mib));
    if (mib_len < 0) {
        printf("Node not found\n");

        return -1;
    }

    printf("%s = ", name);

    if (sysctloidfmt(mib, mib_len, fmt, &kind)) {
        printf("Invalid node\n");

        return -1;
    }
    if (sysctl(mib, mib_len, 0, &dlen, 0, 0)) {
        printf("Invalid node\n");

        return -1;
    }

    ctltype = (kind & CTLTYPE);
    switch (ctltype) {
    case CTLTYPE_STRING:
        getset_svalue(mib, mib_len, dlen, value, strnlen(value, 80));
        break;
    case CTLTYPE_INT:
    case CTLTYPE_UINT:
        getset_ivalue(mib, mib_len, dlen, value, sizeof(int));
        break;
    case CTLTYPE_LONG:
    case CTLTYPE_ULONG:
    case CTLTYPE_S64:
    case CTLTYPE_U64:
        printf("Data type not supported yet\n");
        break;
    }

    return 0;
}

static void getset_svalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len)
{
    char ovalbuf[oval_len + 1];

    if (sysctl(oid, len, ovalbuf, &oval_len, (void *)nval, nval_len)) {
        /* Failed, errno set */
        return;
    }
    printf("%s\n", ovalbuf);
}

static void getset_ivalue(int * oid, int len, size_t oval_len,
        char * nval, size_t nval_len)
{
    int x;
    size_t x_len = sizeof(x);

    /* Get value */
    if (sysctl(oid, len, &x, &x_len, 0, 0)) {
        /* Failed, errno set */
        return;
    }
    printf("%u\n", x);

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

    while ((len_next = sizeof(mib)),
            !(err = sysctlgetnext(mib, len, mib, &len_next))) {
        len = len_next;
        print_mib_name(mib, len);
    }
    if (errno == ENOENT)
        errno = 0;
}

static void print_mib_name(int * mib, int len)
{
    char strname[40];
    size_t strname_len = sizeof(strname);

    sysctlmibtoname(mib, len, strname, &strname_len);
    printf("%s\n", strname);
}

static int tish_uname(char ** args)
{
    char * arg = strtok_r(0, DELIMS, args);
    int mib[2];
    int len;
    size_t str_len;
    char type[20];
    char rele[20] = "";
    char vers[20] = "";

    len = sysctlnametomib("kern.ostype", mib, num_elem(mib));
    str_len = sizeof(type);
    sysctl(mib, len, &type, &str_len, 0, 0);

    if (!strcmp(arg, "-a")) {
        len = sysctlnametomib("kern.osrelease", mib, num_elem(mib));
        str_len = sizeof(rele);
        sysctl(mib, len, &rele, &str_len, 0, 0);

        len = sysctlnametomib("kern.version", mib, num_elem(mib));
        str_len = sizeof(vers);
        sysctl(mib, len, &vers, &str_len, 0, 0);
    }

    printf("%s %s %s\n", type, rele, vers);

    return 0;
}
TISH_CMD(tish_uname, "uname");

static int tish_ikut(char ** arg)
{
    int mib_test[5];
    int mib_cur[5];
    int mib_next[5];
    size_t len_cur, len_next;
    int  err;
    const int one = 1;

    const size_t len_test = sysctlnametomib("debug.test", mib_test,
            num_elem(mib_test));

    printf("     \n"); /* Hack */
    print_mib_name(mib_test, len_test);

    memcpy(mib_cur, mib_test, len_test * sizeof(int));
    len_cur = len_test;

    while ((len_next = sizeof(mib_next)),
            (err = sysctlgetnext(mib_cur, len_cur, mib_next, &len_next)) == 0) {
        if (!sysctltstmib(mib_next, mib_test, len_test)) {
            printf("End of tests\n");
            break; /* EOF debug.test */
        }
        memcpy(mib_cur, mib_next, len_next * sizeof(int));
        len_cur = len_next;

        print_mib_name(mib_cur, len_cur);
        sysctl(mib_cur, len_cur, 0, 0, (void *)(&one), sizeof(one));
    }

    printf("errno = %i\n", errno);
    return 0;
}
TISH_CMD(tish_ikut, "ikut");
