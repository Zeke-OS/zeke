/**
 *******************************************************************************
 * @file arm11_cpu.S
 * @author Olli Vanhoja
 * @brief Some core specific functions needed by the kernel.
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


    .syntax unified
    .text
    .arch armv6k

/**
 * Set TTB.
 * @param ppt   Physical address of the process page table.
 * @param dpgrp Process group changed. 0 if not changed; value other than
 *              zero if changed.
 */
@ void cpu_set_ttb(uint32_t ppt, int dpgrp)
@
    .globl  cpu_set_ttb
    .type   cpu_set_ttb,%function
cpu_set_process:
    mcr     p15, 0, r0, c2, c0, 0   /* Set new TTB value. */
    /* Invalidate TLBs if dgrp != 0 */
    mov     r0, #0
    cmp     r1, #0
    mcrne   p15, 0, r0, c8, c7, 0   /* Invalidate all I+D TLBs. */
    mcrne   p15, 0, r0, c7, c10, 4  /* Drain write buffer. */

.end
