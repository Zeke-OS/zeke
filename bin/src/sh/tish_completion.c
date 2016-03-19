/**
 *******************************************************************************
 * @file    tish_completion.c
 * @author  Olli Vanhoja
 * @brief   Tiny shell.
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

#include <dirent.h>
#include <eztrie.h>
#include <fcntl.h>
#include <limits.h>
#include <linenoise.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tish.h"

static struct eztrie cmd_trie;

static void init_static_completion(void)
{
    struct tish_builtin ** cmd;

    cmd_trie = eztrie_create();
    SET_FOREACH(cmd, tish_cmd) {
        eztrie_insert(&cmd_trie, (*cmd)->name, NULL);
    }
}

static const char * next_path(const char * cp, char * path)
{
    char * s = path;

    while (*cp && *cp != ':') {
        *s++ = *cp++;
    }
    *s = '\0';

    return *cp ? ++cp : NULL;
}

static void init_path_completion(void)
{
    static char dirpath[PATH_MAX];
    const char * pathstr;
    const char * cp;

    pathstr = getenv("PATH");
    if (!pathstr)
        pathstr = _PATH_STDPATH;
    cp = pathstr;

    do {
        int fildes, count;
        struct dirent dbuf[10];

        cp = next_path(cp, dirpath);

        fildes = open(dirpath, O_DIRECTORY | O_RDONLY | O_SEARCH);
        if (fildes < 0)
            continue;

        while ((count = getdents(fildes, (char *)dbuf, sizeof(dbuf))) > 0) {
            for (int i = 0; i < count; i++) {
                struct stat stat;

                fstatat(fildes, dbuf[i].d_name, &stat, 0);
                if ((stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) {
                     eztrie_insert(&cmd_trie, dbuf[i].d_name, NULL);
                }
            }
        }

        close(fildes);
    } while (cp);
}

void tish_completion_init(void)
{
    init_static_completion();
    init_path_completion();
}

void tish_completion(const char * buf, linenoiseCompletions * lc)
{
    struct eztrie_iterator it;
    struct eztrie_node_value * value;

    it = eztrie_find(&cmd_trie, buf);
    while ((value = eztrie_remove_ithead(&it))) {
        linenoiseAddCompletion(lc, value->key);
    }
}
