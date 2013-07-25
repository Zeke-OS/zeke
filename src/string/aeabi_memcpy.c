/**
 *******************************************************************************
 * @file    aeabi_memcpy.c
 * @author  Olli Vanhoja
 *
 * @brief   Wrapper for __aeabi_memcpy as clang expects it to exist even if
 *          there is no implementation for it in libc. __aeabi_memcpy and
 *          some variants of it are usually only implemented if cpu core has
 *          memcpy instructions.
 *******************************************************************************
 */

#include "string.h"

/* Wrapper for aeabi_memcpy */
void __aeabi_memcpy(void *dest, const void *src, ksize_t n)
{
    memcpy (dest, src, n);
}
