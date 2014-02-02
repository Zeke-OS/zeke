/**
 *******************************************************************************
 * @file    _kservices.h
 * @author  Olli Vanhoja
 * @brief   Memory mapped kernel services for userland.
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

#include <pthread.h>

/* This file defines addresses to some syscall wrappers compiled in kernel and
 * mapped to the user space of Zeke. These definitions are intended to work like
 * normal libc calls in user space. Intention of this interface is to provide a
 * light weight easy to wrap POSIX like syscall interface instead of writing
 * the same code for user space and kernel space.
 */

#define _KSERVICES_SHARED_START 0x00080000
uintptr_t __text_shared_start __attribute__((weak));
#define _ssfnaddr(x) ((uintptr_t)(x) - __text_shared_start + _KSERVICES_SHARED_START)

#define _pthread_create                                                 \
    ((int (*)(pthread_t *, const pthread_attr_t *, void * (*)(void *),  \
        void *))_ssfnaddr(pthread_create))                              \

#define _pthread_self                               \
    ((pthread_t (*)(void))_ssfnaddr(pthread_self))

#define _pthread_exit                               \
    ((void (*)(void *))_ssfnaddr(pthread_exit))

