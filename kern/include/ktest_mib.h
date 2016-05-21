/**
 *******************************************************************************
 * @file    ktest_mib.h
 * @author  Olli Vanhoja
 * @brief   MIB decls for unit tests.
 * @section LICENSE
 * Copyright (c) 2014, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#ifndef KTEST_MIB_H
#define KTEST_MIB_H

#include <sys/sysctl.h>

#ifdef configKUNIT_FS
SYSCTL_DECL(_debug_test_fs);
#endif
#ifdef configKUNIT_GENERIC
SYSCTL_DECL(_debug_test_generic);
#endif
#ifdef configKUNIT_HAL
SYSCTL_DECL(_debug_test_hal);
#endif
#ifdef configKUNIT_KSTRING
SYSCTL_DECL(_debug_test_kstring);
#endif
#ifdef configKUNIT_RCU
SYSCTL_DECL(_debug_test_rcu);
#endif
#ifdef configKUNIT_VM
SYSCTL_DECL(_debug_test_vm);
#endif

#endif /* KTEST_MIB_H */
