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

#include <mount.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include "utils.h"

struct optarr optnames[] = {
    { MNT_RDONLY,       "ro" },
    { MNT_SYNCHRONOUS,  "sync" },
    { MNT_ASYNC,        "async" },
    { MNT_NOEXEC,       "noexec" },
    { MNT_NOSUID,       "nosuid" },
    { MNT_NOATIME,      "noatime" },
};

static void usage(void)
{
    fprintf(stderr,
            "usage: mount [-rw] [-o options] [-t type] [source] dest\n");
    exit(EX_USAGE);
}

int main(int argc, char * argv[])
{
    int ch, flags = 0;
    char * vfstype = NULL;
    char * options = NULL;
    char * src = "";
    char * dst = "";

    while ((ch = getopt(argc, argv, "o:rwt:")) != EOF) {
        switch (ch) {
        case 'o':
            if (*optarg)
                options = catopt(NULL, optarg);
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

    if (argc == 1) {
        dst = argv[0];
    } else if (argc == 2) {
        src = argv[0];
        dst = argv[1];
    } else {
        usage();
    }

    flags |= opt2flags(optnames, num_elem(optnames), &options);

    printf("mount: flags: %d, options: \"%s\", vfstype: \"%s\" "
            "src: \"%s\", dst: \"%s\"\n",
            flags, options, vfstype, src, dst);

    if (mount(src, dst, vfstype, flags, options)) {
        perror("mount: failed");
        return EX_OSERR;
    }

    return 0;
}
