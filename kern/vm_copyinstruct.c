/**
 *******************************************************************************
 * @file    vm_copyinstruct.c
 * @author  Olli Vanhoja
 * @brief   vm copyinstruct() for copying structs from user space to kernel
 *          space.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <stddef.h>
#include <stdarg.h>
#include <libkern.h>
#include <dllist.h>
#include <errno.h>
#include <kmalloc.h>
#include <kerror.h>
#include <vm/vm.h>
#include <vm/vm_copyinstruct.h>

struct _cpyin_struct {
    llist_t * gc_lst;
    char data[0];
};

struct _cpyin_gc_node {
    llist_nodedsc_t llist_node;
    char data[0];
};

/**
 * Usage: copyinst(usr, &kern, sizeof(usr), GET_STRUCT_OFFSETS(struct x, a, a_len, c, c_len));
 */
int copyinstruct(__user void * usr, __kernel void ** kern, size_t bytes, ...)
{
    va_list ap;
    struct _cpyin_struct * token;
    size_t i = 0;
    int retval = 0;

    /* Copy the base struct */
    if (!useracc(usr, bytes, VM_PROT_READ))
        return -EFAULT;
    token = kmalloc(sizeof(struct _cpyin_struct) + bytes);
    if (!token)
        return -ENOMEM;
    token->gc_lst = dllist_create(struct _cpyin_gc_node, llist_node);
    *kern = token->data;
    copyin(usr, *kern, bytes);

    /* Copy arguments */
    va_start(ap, bytes);
    while (1) {
        struct _cpyin_gc_node * gc_node;
        const size_t offset = va_arg(ap, size_t);
        void ** src;
        size_t len = va_arg(ap, size_t);
        void * dst;

        if ((offset == 0) && (len == 0) && (i != 0))
            break;
        i++; /* This must be here to prevent it from getting optimized out as a
              * break condition. */

        src = ((void **)((size_t)(*kern) + offset));
        len = *((size_t *)((size_t)(*kern) + len));

        if (len == 0) {
            *src = NULL;
            continue;
        }

        if (!useracc((__user void *)(*src), len, VM_PROT_READ)) {
            retval = -EFAULT;
            break;
        }

        gc_node = kmalloc(sizeof(struct _cpyin_gc_node) + len);
        if (!gc_node) {
            retval = -ENOMEM;
            break;
        }
        token->gc_lst->insert_tail(token->gc_lst, gc_node);
        dst = gc_node->data;

        copyin((__user void *)(*src), dst, len);
        *src = dst;
    };
    va_end(ap);

    return retval;
}

void freecpystruct(void * p)
{
    struct _cpyin_struct * token;
    llist_t * lst;

    if (!p)
        return;

    token = container_of(p, struct _cpyin_struct, data);
    lst = token->gc_lst;

    if (lst && lst->head) {
        struct _cpyin_gc_node * node;

        while ((node = lst->remove(lst, lst->head))) {
            kfree(node);
        }
    }

    dllist_destroy(lst);
    kfree(token);
}
