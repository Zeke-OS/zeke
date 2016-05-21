/**
 *******************************************************************************
 * @file    ktest_mib.c
 * @author  Olli Vanhoja
 * @brief   MIB decls for unit tests.
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

#include <kunit.h>
#include "ktest_mib.h"

#ifdef configKUNIT_FS
SYSCTL_NODE(_debug_test, OID_AUTO, fs, CTLFLAG_RW, 0,
            "fs unit tests");
#endif

#ifdef configKUNIT_GENERIC
SYSCTL_NODE(_debug_test, OID_AUTO, generic, CTLFLAG_RW, 0,
            "generic unit tests");
#endif

#ifdef configKUNIT_HAL
SYSCTL_NODE(_debug_test, OID_AUTO, hal, CTLFLAG_RW, 0,
            "hal unit tests");
#endif

#ifdef configKUNIT_KSTRING
SYSCTL_NODE(_debug_test, OID_AUTO, kstring, CTLFLAG_RW, 0,
            "kstring unit tests");
#endif

#ifdef configKUNIT_RCU
SYSCTL_NODE(_debug_test, OID_AUTO, rcu, CTLFLAG_RW, 0,
            "rcu unit tests");
#endif

#ifdef configKUNIT_VM
SYSCTL_NODE(_debug_test, OID_AUTO, vm, CTLFLAG_RW, 0,
            "vm unit tests");
#endif
