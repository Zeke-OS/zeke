/**
 *******************************************************************************
 * @file    kobj.h
 * @author  Olli Vanhoja
 * @brief   Generic kernel object interface.
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

/**
 * @addtogroup kobj
 * @{
 */

#ifndef _KOBJ_H_
#define _KOBJ_H_

#include <machine/atomic.h>

/**
 * kobj descriptor struct.
 */
struct kobj {
    void (*ko_free)(struct kobj *);
    atomic_t ko_flags;
    atomic_t ko_fast_lock;
    atomic_t ko_refcount;
};

/**
 * Initialize a kobj object descriptor.
 * @param p is a pointer to the kobj object descriptor.
 * @param ko_free is a function pointer to the free function for the object.
 */
void kobj_init(struct kobj * p, void (*ko_free)(struct kobj *));

/**
 * Get refcount of a kobj object descriptor.
 * @param p is a pointer to the kobj object.
 */
int kobj_refcnt(struct kobj * p);

/**
 * Increment the refcount of a kobj object descriptor.
 * @param p is a pointer to the kobj object descriptor.
 * @return Returns zero if succeed; Otherwise a negative ernno is returned.
 */
int kobj_ref(struct kobj * p)
    __attribute__((warn_unused_result));

/**
 * Decrement the refcount of a kobj object descriptor.
 * @param p is a pointer to the kobj object descriptor.
 */
void kobj_unref(struct kobj * p);

/**
 * Increase the refcount of a kobj by count.
 * @param p is a pointer to the kobj object descriptor.
 * @return Returns zero if succeed; Otherwise a negative ernno is returned.
 */
int kobj_ref_v(struct kobj * p, unsigned count)
    __attribute__((warn_unused_result));

/**
 * Decrease the refcount of a kobj by count.
 * @param p is a pointer to the kobj object descriptor.
 * @return Returns zero if succeed; Otherwise a negative ernno is returned.
 */
void kobj_unref_p(struct kobj * p, unsigned count);

/**
 * Decrement the refcount of a kobj object descriptor and
 * prevent further increments.
 * @param p is a pointer to the kobj object descriptor.
 */
void kobj_destroy(struct kobj * p);

#endif /* _KOBJ_H_ */

/**
 * @}
 */
