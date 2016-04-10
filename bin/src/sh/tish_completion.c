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

#include <stdio.h>

#include <ctype.h>
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
#include "../utils.h"

static struct eztrie cmd_trie;

static void init_static_completion(void)
{
    struct tish_builtin ** cmd;

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
    cmd_trie = eztrie_create();
    init_static_completion();
    init_path_completion();
}

void tish_completion_destroy(void)
{
    eztrie_destroy(&cmd_trie);
}

static void completion_cmd(const char * buf, linenoiseCompletions * lc)
{
    struct eztrie_iterator it;
    struct eztrie_node_value * value;

    it = eztrie_find(&cmd_trie, buf);
    while ((value = eztrie_remove_ithead(&it))) {
        linenoiseAddCompletion(lc, value->key);
    }
}

/**
 * @param dir is the original dir.
 * @param dst is the destination buffer.
 * @returns The final search key.
 */
static char * get_bdir(char * dst, const char * dir)
{
    char * s = dst;

    s = strrchr(dir, '/');
    if (s == dir) { /* Begins with "/". */
        dst[0] = '/';
        dst[1] = '\0';
        strcpy(dst + 2, dir + 1);

        return dst + 2;
    } else if (s) { /* Begins some actual path. */
        size_t off = s - dir;
        char * key = dst + off + 2;

        memcpy(dst, dir, off + 1);
        strcpy(key, dir + off + 1);

        return key;
    } else { /* Doesn't begin with any detectable path. */
        char * key = dst + 3;

        strcpy(key, dir);
        dst[0] = '.';
        dst[1] = '/';
        dst[2] = '\0';

        return key;
    }
}

static void completion_dir(const char * buf, const char * d,
                           linenoiseCompletions * lc)
{
    int fildes, count;
    char * bdir = NULL;
    struct eztrie dir_trie;
    struct dirent dbuf[10];
    struct eztrie_iterator it;
    struct eztrie_node_value * value;

    dir_trie = eztrie_create();
    bdir = calloc(strlen(d) + 3, sizeof(char));

    d = get_bdir(bdir, d);

    fildes = open(bdir, O_DIRECTORY | O_RDONLY | O_SEARCH);
    if (fildes < 0) {
        goto out;
    }

    while ((count = getdents(fildes, (char *)dbuf, sizeof(dbuf))) > 0) {
        for (int i = 0; i < count; i++) {
            eztrie_insert(&dir_trie, dbuf[i].d_name, NULL);
        }
    }

    close(fildes);

    it = eztrie_find(&dir_trie, d);
    while ((value = eztrie_remove_ithead(&it))) {
        char * cbuf;

        cbuf = malloc(strlen(buf) + strlen(bdir) + strlen(value->key) + 3);
        if (!cbuf)
            continue;

        sprintf(cbuf, "%s %s%s", buf, bdir, value->key);
        linenoiseAddCompletion(lc, cbuf);

        free(cbuf);
    }

out:
    eztrie_destroy(&dir_trie);
    free(bdir);
}

static char * last_space(char * str)
{
    size_t j = strlen(str);
    char * s = NULL;

    for (int i = j; i >= 0; i--) {
        if (isspace(str[i])) {
            s = str + i;
            break;
        }
    }

    return s;
}

void tish_completion(const char * buf, linenoiseCompletions * lc)
{
    char * cmdbuf;
    char * s;

    cmdbuf = malloc(strlen(buf) + 1);
    if (!cmdbuf)
        return;
    strcpy(cmdbuf, buf);
    s = last_space(cmdbuf);

    if (s) {
        s[0] = '\0';
        s++;
        completion_dir(cmdbuf, s, lc);
    } else {
        completion_cmd(cmdbuf, lc);
    }

    free(cmdbuf);
}
