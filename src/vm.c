/**
 *******************************************************************************
 * @file    vm.c
 * @author  Olli Vanhoja
 * @brief   VM.
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

#include <hal/mmu.h>
#include <ptmapper.h>
#include <kerror.h>
#include <vm/vm.h>

extern mmu_region_t mmu_region_kernel;

/**
 * Check kernel space memory region for accessibility.
 * Check whether operations of the type specified in rw are permitted in the
 * range of virtual addresses given by addr and len.
 * @todo Not implemented properly!
 * @return  Boolean true if the type of access specified by rw is permitted;
 *          Otherwise boolean false.
 */
int kernacc(void * addr, int len, int rw)
{
    size_t reg_start, reg_size;

    reg_start = mmu_region_kernel.vaddr;
    reg_size = mmu_sizeof_region(&mmu_region_kernel);
    if ((addr >= reg_start) && (addr <= reg_start + reg_size))
        return (1 == 1);

    /* TODO Check other regions somehow */
    KERROR(KERROR_WARN, "Cant' verify address in kernacc()");

    return (1 == 1); /* TODO change to 0 */
}

/**
 * Check user space memeory region for accessibility.
 * Check whether operations of the type specified in rw are permitted in the
 * range of virtual addresses given by addr and len.
 * This function considers addr to represent an user space address. The process
 * context to use for this operation is taken from the global variable curproc.
 * @todo Not implemented properly!
 * @return  Boolean true if the type of access specified by rw is permitted;
 *          Otherwise boolean false.
 */
int useracc(void * addr, int len, int rw)
{
    return (1 == 1);
}
