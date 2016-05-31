/**
 *******************************************************************************
 * @file    klocks_isema.h
 * @author  Olli Vanhoja
 * @brief   Kernel space index semaphore.
 * @section LICENSE
 * Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <hal/core.h>
#include <klocks.h>

void isema_init(isema_t * isema, size_t isema_n)
{
    size_t i;

    for (i = 0; i < isema_n; i++) {
        isema[i] = ATOMIC_INIT(0);
    }
}

size_t isema_acquire(isema_t * isema, size_t isema_n)
{
    size_t index = 0;

    do {
        size_t k;

        for (k = 0; k < isema_n; k++) {
            int old = atomic_set(&isema[k], 1);
            if (old == 0) {
                index = k;
                break;
            }
        }
#ifdef configMP
        if (index != 0)
            break;
        cpu_wfe(); /* Sleep until an event is signaled. */
#endif
    } while (index == 0);

    return index;
}
