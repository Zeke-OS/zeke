/**
 *******************************************************************************
 * @file    proc.h
 * @author  Olli Vanhoja
 * @brief   Userland process management.
 * @section LICENSE
 * Copyright (c) 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include <stddef.h>
#include <sys/types.h>

/**
 * Process stat returned by sysctl.
 */
struct kinfo_proc {
    char name[16];
    pid_t pid;
    pid_t pgrp;
    pid_t sid;
    dev_t ctty; /*!< Controlling TTY */
    uid_t ruid;
    uid_t euid;
    uid_t suid;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;
    clock_t utime;
    clock_t stime;
    void * brk_start; /*!< Break start address. (end of heap data) */
    void * brk_stop; /*!< Break stop address. (end of heap region) */
};

struct kinfo_vmentry {
    uintptr_t reg_start;
    uintptr_t reg_end;
    char uap[5];
};

#endif /* _SYS_PROC_H_ */
