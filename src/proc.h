/**
 *******************************************************************************
 * @file    proc.h
 * @author  Olli Vanhoja
 * @brief   Kernel process management header file.
 * @section LICENSE
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#ifndef PROC_H
#define PROC_H

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

#if configMP != 0
#include <klocks.h>
#endif

/**
 * Process Control Block or Process Descriptor Structure
 */
typedef struct {
    pid_t pid;
#if 0
    char * command;
#endif
    threadInfo_t * main_thread; /*!< Main thread of this process. */
#ifndef PU_TEST_BUILD
    struct mm {
        mmu_pagetable_t pptable;    /*!< Process master page table. */
        mmu_region_t * regions;   /*!< Memory regions of a process.
                                     *   [0] = code
                                     *   [1] = stack
                                     *   [2] = heap/data
                                     *   [n] = allocs
                                     */
        int nr_regions;             /* Number of regions allocated. */
    } mm;
#endif
    sigs_t sigs;                /*!< Signals. */

    /* TODO - note: main_thread already has a liked list of child threads
     *      - file_t fd's
     *      - tty
     *      - etc.
     */

    /**
     * Process inheritance; Parent and child thread pointers.
     * inh : Parent and child process relations
     * ----------------------------------------
     * + first_child is a parent process attribute containing address to a first
     *   child of the parent process
     * + parent is a child process attribute containing address of the parent
     *   process of the child thread
     * + next_child is a child thread attribute containing address of a next
     *   child node of the common parent thread
     */
    struct inh {
        void * parent;      /*!< Parent thread. */
        void * first_child; /*!< Link to the first child thread. */
        void * next_child;  /*!< Next child of the common parent. */
    } inh;
#if configMP != 0
    mtx_t plock;
#endif
} proc_info_t;

int maxproc;
extern volatile pid_t current_process_id;
extern volatile proc_info_t * curproc;

pid_t proc_fork(pid_t pid);
int proc_kill(void);
int proc_replace(pid_t pid, void * image, size_t size);
proc_info_t * proc_get_struct(pid_t pid);
mmu_pagetable_t * pr_get_pptable(pid_t pid);

#endif /* PROCESS_H */

/**
  * @}
  */

