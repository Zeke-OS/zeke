/**
 *******************************************************************************
 * @file    pthreads_keys.c
 * @author  Olli Vanhoja
 * @brief   Thread-specific data key management.
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

#include <errno.h>
#include <machine/atomic.h>
#include <sys/tree.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>

#define KEY_RSV (void (*)(void *))(1)

struct ptkey_arr {
    pthread_t tid;
    const void * values[PTHREAD_KEYS_MAX];
    RB_ENTRY(ptkey_arr) _entry;
};

/**
 * Pthread keys by thread id.
 */
static RB_HEAD(ptkeyvals, ptkey_arr) ptkeyvals_head = RB_INITIALIZER(_head);

/**
 * Pointers to destructors.
 */
static void (*keys[PTHREAD_KEYS_MAX])(void *);

static int ptkey_comp(struct ptkey_arr * a, struct ptkey_arr * b);
RB_PROTOTYPE_STATIC(ptkeyvals, ptkey_arr, _entry, ptkey_comp);
static struct ptkey_arr * get_ptkey_arr(void);

static int ptkey_comp(struct ptkey_arr * a, struct ptkey_arr * b)
{
    return (int)a->tid - (int)b->tid;
}

RB_GENERATE_STATIC(ptkeyvals, ptkey_arr, _entry, ptkey_comp);

static struct ptkey_arr * get_ptkey_arr(void)
{
    struct ptkey_arr find;
    struct ptkey_arr * elm;

    find.tid = pthread_self();

    if (RB_EMPTY(&ptkeyvals_head) ||
            !(elm = RB_FIND(ptkeyvals, &ptkeyvals_head, &find))) {
        elm = malloc(sizeof(struct ptkey_arr));
        if (!elm) {
            errno = ENOMEM;
            return NULL;
        }

        elm->tid = find.tid;
        memset(elm->values, 0, sizeof(find.values));
        RB_INSERT(ptkeyvals, &ptkeyvals_head, elm);
    }

    return elm;
}

int pthread_key_create(pthread_key_t * key, void (*destructor)(void*))
{
    size_t i;
    struct ptkey_arr * ptkey_arr;

    ptkey_arr = get_ptkey_arr();
    if (!ptkey_arr)
        return errno;

    if (!destructor)
        destructor = KEY_RSV;

    for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
        if (keys[i] == NULL) {
            if (atomic_cmpxchg_ptr(&keys[i], NULL, destructor) == NULL) {
                /* Succeed. */
                *key = i;
                return 0;
            }
        }
    }

    errno = EAGAIN;
    return EAGAIN;
}

int pthread_key_delete(pthread_key_t key)
{
    struct ptkey_arr * elm;

    if (RB_EMPTY(&ptkeyvals_head) || !(elm = get_ptkey_arr())) {
        errno = EINVAL;
        return EINVAL;
    }

    elm->values[key] = NULL;
    keys[key] = NULL;

    return 0;
}

void * pthread_getspecific(pthread_key_t key)
{
    struct ptkey_arr * elm;

    if (RB_EMPTY(&ptkeyvals_head) || !(elm = get_ptkey_arr())) {
        return NULL;
    }

    return (void *)elm->values[key];
}

int pthread_setspecific(pthread_key_t key, const void * value)
{
    struct ptkey_arr * elm;

    if (RB_EMPTY(&ptkeyvals_head)) {
        errno = EINVAL;
        return EINVAL;
    }

    if (!(elm = get_ptkey_arr())) {
        return errno;
    }

    elm->values[key] = value;

    return 0;
}

void __pthread_key_dtors(void)
{
    size_t i, j, nf = 1;
    struct ptkey_arr * elm;

    if (RB_EMPTY(&ptkeyvals_head) || !(elm = get_ptkey_arr())) {
        return;
    }

    for (j = 0; nf && (j < PTHREAD_DESTRUCTOR_ITERATIONS); j++) {
        nf = 0;
        for (i = 0; i < PTHREAD_KEYS_MAX; i++) {
            if (((uintptr_t)keys[i] > (uintptr_t)KEY_RSV) && elm->values[i]) {
                void * tmp;

                tmp = (void *)elm->values[i];
                elm->values[i] = NULL;
                keys[i](tmp);

                nf = 1;
            }
        }
    }

    RB_REMOVE(ptkeyvals, &ptkeyvals_head, elm);
    free(elm);
}
