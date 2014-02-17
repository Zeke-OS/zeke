/**
 *******************************************************************************
 * @file    mmu.h
 * @author  Olli Vanhoja
 * @brief   MMU headers.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef KINIT_H
#define KINIT_H

#include <kerror.h>

/**
 * Subsystem initializer prologue.
 * @todo TODO Fix order of init messages
 */
#define SUBSYS_INIT()               \
    static int __subsys_init = 0;   \
    do {                            \
    if (__subsys_init != 0) return; \
    else __subsys_init = 1;         \
    } while (0)

#define SUBSYS_INITFINI(msg)        \
    KERROR(KERROR_LOG, msg)

/**
 * Subsystem initializer dependency.
 * Mark that subsystem initializer depends on dep.
 * @param dep is a name of an intializer function.
 */
#define SUBSYS_DEP(dep)             \
    extern void dep(void);          \
    dep()

/**
 * hw_preinit initializer functions are run before any other kernel initializer
 * functions.
 */
#define HW_PREINIT_ENTRY(fn) static void (*fp_##fn)(void) __attribute__ ((section (".hw_preinit_array"), __used__)) = fn;

/**
 * hw_post_init initializer are run after all other kernel initializer so post
 * init is ideal for example initializing hw timers and interrupts.
 */
#define HW_POSTINIT_ENTRY(fn) static void (*fp_##fn)(void) __attribute__ ((section (".hw_postinit_array"), __used__)) = fn;

#endif
