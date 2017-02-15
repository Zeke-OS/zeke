/**
 *******************************************************************************
 * @file    tish_sysctl.c
 * @author  Olli Vanhoja
 * @brief   Tiny Init Shell for debugging in init.
 * @section LICENSE
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

static int getset_svalue(int * oid, int len, size_t oval_len,
                         char * nval, size_t nval_len)
{
    char ovalbuf[oval_len + 1];

    if (sysctl(oid, len, ovalbuf, &oval_len, NULL, 0))
        return 1;

    printf("%s\n", ovalbuf);

    if (sysctl(oid, len, NULL, NULL, (void *)nval, nval_len)) {
        fprintf(stderr, "Failed to write\n");
        return 1;
    }

    return 0;
}

static int getset_ivalue(int * oid, int len, size_t oval_len,
                         char * nval, size_t nval_len)
{
    int x;
    size_t x_len = sizeof(x);

    /* Get value */
    if (sysctl(oid, len, &x, &x_len, NULL, 0))
        return 1;
    printf("%i\n", x);

    /* Set value */
    if (nval) {
        x = atoi(nval);
        if (sysctl(oid, len, NULL, NULL, (void *)(&x), sizeof(int))) {
            fprintf(stderr, "Failed to write\n");
            return 1;
        }
    }

    return 0;
}

static void print_mib_name(int * mib, int len)
{
    char strname[40];
    size_t strname_len = sizeof(strname);

    sysctlmibtoname(mib, len, strname, &strname_len);
    printf("%s\n", strname);
}

static int print_tree(int * mib_start, int len_start)
{
    int mib[CTL_MAXNAME];
    int err, len = len_start;
    size_t len_next;

#if 0
    printf("mib[%d]:", len_start);
    for (size_t i = 0; i < len_start; i++) {
        printf(" %d", mib_start[i]);
    }
    printf("\n");
#endif

    memset(mib, 0, CTL_MAXNAME);
    memcpy(mib, mib_start, len_start);

    while ((len_next = sizeof(mib)),
            !(err = sysctlgetnext(mib, len, mib, &len_next))) {
        len = len_next;

        if (memcmp(mib, mib_start, len_start * sizeof(int)) == 0) {
            print_mib_name(mib, len);
        }

    }
    if (err && errno == ENOENT)
        errno = 0;
    return errno ? 1 : 0;
}

static int cmd_mib_value(char * name, int ctltype, int * mib, int mib_len,
                         char * new_value)
{
    size_t dlen;

    printf("%s = ", name);

    if (sysctl(mib, mib_len, 0, &dlen, 0, 0)) {
        fprintf(stderr, "Invalid node\n");

        return 1;
    }

    switch (ctltype) {
    case CTLTYPE_STRING:
        return getset_svalue(mib, mib_len, dlen, new_value,
                             new_value ? strlen(new_value) : 0);
    case CTLTYPE_INT:
    case CTLTYPE_UINT:
        return getset_ivalue(mib, mib_len, dlen, new_value, sizeof(int));
    case CTLTYPE_LONG:
    case CTLTYPE_ULONG:
    case CTLTYPE_S64:
    case CTLTYPE_U64:
        fprintf(stderr, "Data type not supported yet\n");
        break;
    }

    return 0;
}

static int getset_parm(char * arg)
{
    char * name;
    char * new_value;
    char * rest = NULL;

    name = strtok_r(arg, "=", &rest);
    new_value = strtok_r(NULL, "=", &rest);
    if (!name) {
        fprintf(stderr, "Invalid argument\n");
        return 1;
    }

    int mib[CTL_MAXNAME];
    const int mib_len = sysctlnametomib(name, mib, num_elem(mib));
    if (mib_len < 0) {
        fprintf(stderr, "Node not found\n");
        return 1;
    }

    char fmt[5];
    unsigned int kind;
    if (sysctloidfmt(mib, mib_len, fmt, &kind)) {
        fprintf(stderr, "Invalid node\n");

        return 1;
    }

    const int ctltype = (kind & CTLTYPE);
    if (ctltype == CTLTYPE_NODE) {
        return print_tree(mib, mib_len);
    } else {
        return cmd_mib_value(name, ctltype, mib, mib_len, new_value);
    }
}

static int tish_sysctl_cmd(char * argv[])
{
    char * arg = argv[1];

    if (arg && !strcmp(arg, "-a")) {
        int mib[CTL_MAXNAME] = {0};

        print_tree(mib, 0);
    } else if (arg) {
        return getset_parm(arg);
    } else {
        fprintf(stderr, "usage: sysctl name[=value]\n"
                        "       sysctl -a\n");
        return 1;
    }
    return 0;
}
TISH_CMD(tish_sysctl_cmd, "sysctl", " <ctlname>", 0);

static int tish_uname(char * argv[])
{
    char * arg = argv[1];
    int mib[2];
    int len;
    size_t str_len;
    char type[20];
    char rele[40] = "";
    char vers[40] = "";

    len = sysctlnametomib("kern.ostype", mib, num_elem(mib));
    str_len = sizeof(type);
    sysctl(mib, len, &type, &str_len, 0, 0);

    if (arg && !strcmp(arg, "-a")) {
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
TISH_CMD(tish_uname, "uname", NULL, 0);

static int tish_ikut(char * argv[])
{
    int mib_test[5];
    int mib_cur[5];
    int mib_next[5];
    size_t len_cur, len_next;
    const int one = 1;

    const size_t len_test = sysctlnametomib("debug.test", mib_test,
            num_elem(mib_test));

    printf("     \n"); /* Hack */
    print_mib_name(mib_test, len_test);

    memcpy(mib_cur, mib_test, len_test * sizeof(int));
    len_cur = len_test;

    while ((len_next = sizeof(mib_next)),
           sysctlgetnext(mib_cur, len_cur, mib_next, &len_next) == 0) {
        if (!sysctltstmib(mib_next, mib_test, len_test)) {
            printf("End of tests\n");
            break; /* EOF debug.test */
        }
        memcpy(mib_cur, mib_next, len_next * sizeof(int));
        len_cur = len_next;

        print_mib_name(mib_cur, len_cur);
        sysctl(mib_cur, len_cur, 0, 0, (void *)(&one), sizeof(one));
    }

    return 0;
}
TISH_CMD(tish_ikut, "ikut", NULL, 0);
