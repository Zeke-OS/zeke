/**
 *******************************************************************************
 * @file    vm_copyinstruct.h
 * @author  Olli Vanhoja
 * @brief   vm copyinstruct() for copying structs from user space to kernel
 *          space.
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

#include <stddef.h>

#define CONCATENATE(arg1, arg2)   CONCATENATE1(arg1, arg2)
#define CONCATENATE1(arg1, arg2)  CONCATENATE2(arg1, arg2)
#define CONCATENATE2(arg1, arg2)  arg1##arg2

#define GET_STRUCT_OFFSETS_1(structure, field, ...) \
    offsetof(structure, field),
#define GET_STRUCT_OFFSETS_2(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_1(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_3(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_2(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_4(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_3(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_5(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_4(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_6(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_5(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_7(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_6(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_8(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_7(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_9(structure, field, ...) \
    offsetof(structure, field),                     \
    GET_STRUCT_OFFSETS_8(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_10(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_9(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_11(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_10(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_12(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_11(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_13(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_12(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_14(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_13(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_15(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_14(structure, __VA_ARGS__)

#define GET_STRUCT_OFFSETS_NARG(...) \
    GET_STRUCT_OFFSETS_NARG_(__VA_ARGS__, GET_STRUCT_OFFSETS_RSEQ_N())
#define GET_STRUCT_OFFSETS_NARG_(...) GET_STRUCT_OFFSETS_ARG_N(__VA_ARGS__)
#define GET_STRUCT_OFFSETS_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, N, ...) N
#define GET_STRUCT_OFFSETS_RSEQ_N() 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define GET_STRUCT_OFFSETS_(N, structure, field, ...) \
    CONCATENATE(GET_STRUCT_OFFSETS_, N)(structure, field, __VA_ARGS__)

#define GET_STRUCT_OFFSETS(structure, field, ...) \
    GET_STRUCT_OFFSETS_(GET_STRUCT_OFFSETS_NARG(field, __VA_ARGS__), \
            structure, field, __VA_ARGS__) 0, 0

/**
 * Usage: copyinst(usr, &kern, sizeof(usr), GET_STRUCT_OFFSETS(struct x, a, a_len, c, c_len));
 */
int copyinstruct(void * usr, void ** kern, size_t bytes, ...);
void freecpystruct(void * p);
