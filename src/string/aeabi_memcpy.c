/**
 *******************************************************************************
 * @file    aeabi_memcpy.c
 * @author  Olli Vanhoja
 *
 * @brief   Wrappers for __aeabi_memcpy routines as clang expects it to exist
 *          even if there is no implementation for it in libc. __aeabi_memcpy
 *          and some variants of it are usually only implemented if cpu core has
 *          memcpy instructions.
 *
 *          Header file is not needed because these are only referenced by
 *          clang and used by linker.
 *******************************************************************************
 */

#include "string.h"

void __aeabi_memcpy(void *destination, const void *source, ksize_t num)
{
    memcpy(destination, source, num);
}

void __aeabi_memcpy4(void *destination, const void *source, ksize_t num)
{
    memcpy(destination, source, num);
}

void __aeabi_memcpy8(void *destination, const void *source, ksize_t num)
{
        memcpy(destination, source, num);
}
