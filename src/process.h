/**
 *******************************************************************************
 * @file    process.h
 * @author  Olli Vanhoja
 * @brief   Kernel process management header file.
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

/** @addtogroup Process
  * @{
  */

#pragma once
#ifndef PROCESS_H
#define PROCESS_H

#include <autoconf.h>

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <kernel.h>
#include <sched.h> /* Needed for threadInfo_t and threading functions */

#if configMMU == 0 && !defined(PU_TEST_BUILD)
#error Processes are not supported without MMU.
#elif defined(PU_TEST_BUILD)
/* Test build */
#else /* Normal build with MMU */
#include <hal/mmu.h>
#endif

/**
 * Process Control Block or Process Descriptor Structure
 */
typedef struct {
    pid_t pid;
    threadInfo_t * main_thread; /*!< Main thread of this process. */
#ifndef PU_TEST_BUILD
    mmu_pagetable_t * pptable;  /*!< Process master page table. */
    mmu_region_t regions[3];    /*!< Standard regions of a process.
                                 *   [0] = stack
                                 *   [1] = heap/data
                                 *   [2] = code
                                 */
#endif
    sigs_t sigs;                /*!< Signals. */

    /* TODO - note: main_thread already has a liked list of child threads
     *      - page table(s)
     *      - memory allocations (that should be freed automatically if process exits)
     *      - file_t fd's
     *      - etc.
     */
} processInfo_t;


extern volatile pid_t current_process_id;


mmu_pagetable_t * process_get_pptable(pid_t pid);

int copyin(const void * uaddr, void * kaddr, size_t len);
int copyout(const void * kaddr, void * uaddr, size_t len);
int copyinstr(const void * uaddr, void * kaddr, size_t len, size_t * done);

#endif /* PROCESS_H */

/**
  * @}
  */

