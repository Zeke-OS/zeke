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

/**
 * Initialize command completion for internal commands of the shell.
 */
static void init_static_completion(void)
{
    const struct tish_builtin ** cmd;

    SET_FOREACH(cmd, tish_cmd) {
        eztrie_insert(&cmd_trie, (*cmd)->name, *cmd);
    }
}

/**
 * Get the next dirpath string in PATH.
 */
static const char * next_path(char * dst, const char * cp)
{
    while (*cp && *cp != ':') {
        *dst++ = *cp++;
    }
    *dst = '\0';

    return *cp ? ++cp : NULL;
}

/**
 * Initialize command completion for executables found in PATH.
 */
static void init_path_completion(void)
{
    char * dirpath; /* Current directory from PATH */
    const char * cp; /* Pointer to the current position in PATH. */

    dirpath = malloc(PATH_MAX);
    if (!dirpath) {
        perror("Unable to initialize PATH completion");
        return;
    }

    cp = getenv("PATH");
    if (!cp)
        cp = _PATH_STDPATH;

    do {
        int fildes, count;
        struct dirent dbuf[10];

        cp = next_path(dirpath, cp);

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

    free(dirpath);
}

/**
 * Completion for incomplete command names.
 */
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
 * Get basedir of an incomplete path.
 * @param[out] dst  is the destination buffer.
 * @param[in] dir   is the original dir.
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
    } else if (s) { /* Begins with an actual path. */
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

/**
 * Completion for path names.
 * @param cmd   is a pointer the user input string preceeding the path to be
 *              completed.
 * @param dir   is a pointer to the string expected to be an incomplete path to
 *              a file or a directory.
 * @param lc    is a pointer to the linenoiseCompletion object related to the
 *              current line.
 */
static void completion_path(const char * cmd, const char * dir,
                            linenoiseCompletions * lc)
{
    int fildes, count;
    char * bdir = NULL;
    struct eztrie dir_trie;
    struct dirent dbuf[10];
    struct eztrie_iterator it;
    struct eztrie_node_value * value;

    dir_trie = eztrie_create();
    bdir = calloc(strlen(dir) + 3, sizeof(char));

    dir = get_bdir(bdir, dir);

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

    it = eztrie_find(&dir_trie, dir);
    while ((value = eztrie_remove_ithead(&it))) {
        char * cbuf;

        cbuf = malloc(strlen(cmd) + strlen(bdir) + strlen(value->key) + 3);
        if (!cbuf) {
            perror("Path completion failed");
            continue;
        }

        if (cmd[0] == '\0') {
            sprintf(cbuf, "%s%s", bdir, value->key);
        } else {
            sprintf(cbuf, "%s %s%s", cmd, bdir, value->key);
        }
        linenoiseAddCompletion(lc, cbuf);

        free(cbuf);
    }

out:
    free(bdir);
    eztrie_destroy(&dir_trie);
}

/**
 * Find the last space in a string.
 * @param str is a pointer to the cstring.
 * @returns Returns a pointer to the last space found in a string;
 *          Otherwise a NULL pointer is returned.
 */
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

static void tish_completion(const char * buf, linenoiseCompletions * lc)
{
    char * cmdbuf;
    char * s;

    cmdbuf = malloc(strlen(buf) + 1);
    if (!cmdbuf) {
        perror("Completion failed");
        return;
    }
    strcpy(cmdbuf, buf);
    s = last_space(cmdbuf);

    if (s) {
        /* Entering the second or n'th word. */
        s[0] = '\0';
        s++;
        completion_path(cmdbuf, s, lc);
    } else if (cmdbuf[0] == '/' || (cmdbuf[0] == '.' && cmdbuf[1] == '/')) {
        /* Entering the first word and we assume it's a path. */
        completion_path("", cmdbuf, lc);
    } else {
        /* Entering the first word and we assume it's a command. */
        completion_cmd(cmdbuf, lc);
    }

    free(cmdbuf);
}

static char * tish_hints(const char * buf, int * color, int * bold)
{
    struct eztrie_iterator it;
    struct eztrie_node_value * tmp;
    struct eztrie_node_value * value;
    size_t count = 0;

    it = eztrie_find(&cmd_trie, buf);
    while ((tmp = eztrie_remove_ithead(&it))) {
        value = tmp;
        count++;
    }
    if (count == 1 && value && value->p) {
        const struct tish_builtin * cmd = value->p;

        *color = 35;
        *bold = 0;
        return cmd->hint;
    }

    return NULL;
}

void tish_completion_init(void)
{
    cmd_trie = eztrie_create();
    init_static_completion();
    init_path_completion();

    linenoiseSetCompletionCallback(tish_completion);
    linenoiseSetHintsCallback(tish_hints);
}

void tish_completion_destroy(void)
{
    eztrie_destroy(&cmd_trie);
}
