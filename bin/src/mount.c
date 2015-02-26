/*
 * Copyright (c) 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1980, 1989, 1993, 1994
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mount.h>

struct opt {
    const int opt;
    const char * optname;
} optnames[] = {
    { MNT_RDONLY,       "ro" },
    { MNT_SYNCHRONOUS,  "sync" },
    { MNT_ASYNC,        "async" },
    { MNT_NOEXEC,       "noexec" },
    { MNT_NOSUID,       "nosuid" },
    { MNT_NOATIME,      "noatime" },
};

static char * catopt(char * s0, char * s1);
static unsigned long opt2flags(char ** options);

static void usage(void)
{
    fprintf(stderr,
            "usage: mount [-rw] [-o options] [-t type] source dest\n");
    exit(1);
}

int main(int argc, char * argv[])
{
    int ch, flags = 0;
    char * vfstype = NULL;
    char * options = NULL;
    char * src;
    char * dst;

    while ((ch = getopt(argc, argv, "o:rwt:")) != EOF) {
        switch (ch) {
        case 'o':
            if (*optarg)
                options = catopt(options, optarg);
            break;
        case 'r':
            flags |= MNT_RDONLY;
            break;
        case 't':
            if (vfstype) {
                fprintf(stderr, "mount: only one -t option may be specified");
                exit(1);
            }
            vfstype = optarg;
        case 'w':
            flags &= ~MNT_RDONLY;
            break;
        case '?':
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (!vfstype)
        vfstype = "auto";

    if (argc < 2) {
        usage();
    }
    src = argv[0];
    dst = argv[1];

    flags |= opt2flags(&options);

#if 0
    printf("mount: flags: %d, options: \"%s\", vfstype: \"%s\" "
            "src: \"%s\", dst: \"%s\"\n",
            flags, options, vfstype, src, dst);
#endif

    if (mount(src, dst, vfstype, flags, options))
        perror("mount: failed");

    return 0;
}

static char * catopt(char * s0, char * s1)
{
    char * cp;

    if (s0 && *s0) {
        cp = malloc(strlen(s0) + strlen(s1) + 1 + 1);
        if (!cp) {
            perror("Failed");
            exit(1);
        }

        sprintf(cp, "%s,%s", s0, s1);
    } else {
        cp = strdup(s1);
    }

    if (s0)
        free(s0);
    return cp;
}

static unsigned long opt2flags(char ** options)
{
    const char delim[] = ",";
    char * option;
    char * lasts;
    char * remains = NULL;
    unsigned long flags = 0;

    option = strtok_r(*options, delim, &lasts);
    if (!option)
        return flags;

    do {
        int match = 0;

        for (size_t i = 0; i < num_elem(optnames); i++) {
            struct opt * o = optnames + i;

            if (strcmp(o->optname, option))
                continue;

            flags |= o->opt;
            match = 1;
            break;
        }
        if (!match)
            remains = catopt(remains, option);
    } while ((option = strtok_r(NULL, delim, &lasts)));

    *options = remains;
    return flags;
}
