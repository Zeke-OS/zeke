/*
 * _PDCLIB_allocpages(int const)
 *
 * This file is part of the Public Domain C Library (PDCLib).
 * Permission is granted to use, modify, and / or redistribute at will.
 */

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/_PDCLIB_glue.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANOYNMOUS
#endif

void * _PDCLIB_allocpages(size_t n)
{
    void *addr = mmap(NULL, n * sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE,
                      MAP_ANON | MAP_PRIVATE, -1, 0);

    if(addr != MAP_FAILED) {
        return addr;
    } else {
        return NULL;
    }
}
