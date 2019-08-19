/**
 *******************************************************************************
 * @file    parsenames.c
 * @author  Olli Vanhoja
 * @brief   Parse path and file name from a complete path.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <limits.h>
#include <kstring.h>
#include <kmalloc.h>
#include <libkern.h>
#include <kerror.h>

int parsenames(const char * pathname, char ** path, char ** name)
{
    char * path_act;
    char * fname;
    size_t i;
    size_t j;
    int retval;

    KASSERT(pathname != NULL, "pathname should be set\n");

    path_act = kstrdup(pathname, PATH_MAX);
    fname = kzalloc(NAME_MAX);
    if (!(path_act && fname)) {
        retval = -ENOMEM;
        goto fail;
    }

    i = strlenn(path_act, PATH_MAX);
    if (path_act[i] != '\0') {
        retval = -ENAMETOOLONG;
        goto fail;
    }

    while (path_act[i] != '/') {
        path_act[i--] = '\0';
        if ((i == 0) &&
            (!(path_act[0] == '/') ||
             !(path_act[0] == '.' && path_act[1] == '/'))) {
            path_act[0] = '.';
            path_act[1] = '/';
            path_act[2] = '\0';
            i--; /* little trick */
            break;
        }
    }

    for (j = 0; j < NAME_MAX;) {
        i++;
        if (pathname[i] == '/')
            continue;

        fname[j] = pathname[i];
        if (fname[j] == '\0')
            break;
        j++;
    }

    if (fname[j] != '\0') {
        retval = -ENAMETOOLONG;
        goto fail;
    }

    if (path) {
        *path = path_act;
        path_act = NULL;
    }

    if (name) {
        *name = fname;
        fname = NULL;
    }

    retval = 0;
fail:
    kfree(path_act);
    kfree(fname);
    return retval;
}
