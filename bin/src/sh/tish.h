/**
 *******************************************************************************
 * @file    tish.h
 * @author  Olli Vanhoja
 * @brief   Tiny Shell.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef TISH_H
#define TISH_H

#include <sys/linker_set.h>
#include <linenoise.h>

#define TISH_NOFORK 0x1

typedef int builtin_cmd_t(char * argv[]);

struct tish_builtin {
    char name[10];
    unsigned flags;
    builtin_cmd_t * fn;
};

SET_DECLARE(tish_cmd, struct tish_builtin);

#define TISH_CMD(fun, cmdnamestr)                   \
    static struct tish_builtin fun##_st = {         \
        .name = cmdnamestr, .flags = 0, .fn = fun   \
    };                                              \
    DATA_SET(tish_cmd, fun##_st)

#define TISH_NOFORK_CMD(fun, cmdnamestr)            \
    static struct tish_builtin fun##_st = {         \
        .name = cmdnamestr, .flags = TISH_NOFORK,   \
        .fn = fun                                   \
    };                                              \
    DATA_SET(tish_cmd, fun##_st)

void tish_completion_init(void);
void tish_completion_destroy(void);
void tish_completion(const char * buf, linenoiseCompletions * lc);

size_t split(char * buffer, char * argv[], size_t argc_max);
void run_line(char * line);

#endif /* TISH_H */
