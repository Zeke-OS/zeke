/**
 *******************************************************************************
 * @file    vm_copyinstruct.h
 * @author  Olli Vanhoja
 * @brief   vm copyinstruct() for copying structs from user space to kernel
 *          space.
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

#pragma once
#ifndef VM_COPYINSTRUCT_H
#define VM_COPYINSTRUCT_H

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
#define GET_STRUCT_OFFSETS_16(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_15(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_17(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_16(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_18(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_17(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_19(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_18(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_20(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_19(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_21(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_20(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_22(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_21(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_23(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_22(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_24(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_23(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_25(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_24(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_26(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_25(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_27(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_26(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_28(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_27(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_29(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_28(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_30(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_29(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_31(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_30(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_32(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_31(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_33(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_32(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_34(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_33(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_35(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_34(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_36(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_35(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_37(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_36(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_38(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_37(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_39(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_38(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_40(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_39(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_41(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_40(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_42(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_41(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_43(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_42(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_44(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_43(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_45(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_44(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_46(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_45(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_47(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_46(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_48(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_47(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_49(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_48(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_50(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_49(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_51(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_50(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_52(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_51(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_53(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_52(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_54(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_51(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_55(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_54(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_56(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_55(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_57(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_56(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_58(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_57(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_59(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_58(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_60(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_59(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_61(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_60(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_62(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_61(structure, __VA_ARGS__)
#define GET_STRUCT_OFFSETS_63(structure, field, ...) \
    offsetof(structure, field),                      \
    GET_STRUCT_OFFSETS_62(structure, __VA_ARGS__)

#define GET_STRUCT_OFFSETS_NARG(...) \
    GET_STRUCT_OFFSETS_NARG_(__VA_ARGS__, GET_STRUCT_OFFSETS_RSEQ_N())
#define GET_STRUCT_OFFSETS_NARG_(...) GET_STRUCT_OFFSETS_ARG_N(__VA_ARGS__)
#define GET_STRUCT_OFFSETS_ARG_N( \
         _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, _11, _12, _13, _14, \
        _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, \
        _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, \
        _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, \
        _57, _58, _59, _60, _61, _62, _63, N, ...) N
#define GET_STRUCT_OFFSETS_RSEQ_N() \
        63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, \
        46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, \
        12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define GET_STRUCT_OFFSETS_(N, structure, field, ...) \
    CONCATENATE(GET_STRUCT_OFFSETS_, N)(structure, field, __VA_ARGS__)

#define GET_STRUCT_OFFSETS(structure, field, ...) \
    GET_STRUCT_OFFSETS_(GET_STRUCT_OFFSETS_NARG(field, __VA_ARGS__), \
            structure, field, __VA_ARGS__) 0, 0

/**
 * Copy in struct and selected members from user space to kernel space.
 * Usage: copyinst(usr, &kern, sizeof(usr),
 *                 GET_STRUCT_OFFSETS(struct x, a, a_len, c, c_len));
 *        Supports upto 63 struct members.
 * @param[in]   usr     is the user space address to the struct.
 * @param [out] kern    is a kernel space pointer that will be set to point to
 *                      the newly copied struct.
 * @param       bytes   is the size of the container struct in bytes.
 * @param       ...     byte offset of members to be copied and sizes of the
 *                      members. (offset, len)
 * @return      0 if succeed;
 *              Otherwise a negative errno value is returned.
 */
int copyinstruct(__user void * usr, __kernel void ** kern, size_t bytes, ...);

/**
 * Free a structure and copied members allocated with copyinstruct().
 * @param[in]   p       is a pointer to the container struct returned by
 *                      copyinstruct().
 */
void freecpystruct(void * p);

#endif /* VM_COPYINSTRUCT_H */
