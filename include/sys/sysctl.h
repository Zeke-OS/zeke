/**
 *******************************************************************************
 * @file    sysctl.h
 * @author  Olli Vanhoja
 *
 * @brief   Sysctl headers.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 1989, 1993
 *        The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *        @(#)sysctl.h        8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_SYSCTL_H_
#define _SYS_SYSCTL_H_

#include <machine/atomic.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#ifndef CTASSERT        /* Allow lint to override */
#define CTASSERT(x)     _Static_assert(x, "compile-time assertion failed")
#endif

/*
 * Definitions for sysctl call.  The sysctl call uses a hierarchical name
 * for objects that can be examined or modified.  The name is expressed as
 * a sequence of integers.  Like a file path name, the meaning of each
 * component depends on its place in the hierarchy.  The top-level and kern
 * identifiers are defined here, and other identifiers are defined in the
 * respective subsystem header files.
 */

#define CTL_MAXNAME     24 /*! Largest number of components supported
                            * (n * sizeof(int)). */

#define CTL_MAXSTRNAME  80 /*! Maximum length of a string name for a sysctl
                            *  node. */

#define CTLT_STRING_MAX 1024

/* CTL types */
#define CTLTYPE         0xf /*!< Mask for the type. */
#define CTLTYPE_NODE    1   /*!< Name is a node (parent for other nodes). */
#define CTLTYPE_INT     2   /*!< Name describes an signed integer */
#define CTLTYPE_STRING  3   /*!< Name describes a string */
#define CTLTYPE_OPAQUE  4   /*!< name describes a structure */
#define CTLTYPE_S64     5   /*!< Name describes a signed 64-bit number */
#define CTLTYPE_UINT    6   /*!< Name describes an unsigned integer */
#define CTLTYPE_LONG    7   /*!< Name describes a long */
#define CTLTYPE_ULONG   8   /*!< Name describes an unsigned long */
#define CTLTYPE_U64     9   /*!< Name describes an unsigned 64-bit number */

/* CTL flags */
#define CTLFLAG_RD      0x10000000  /*!< Allow reads of variable */
#define CTLFLAG_WR      0x20000000  /*!< Allow writes to the variable */
#define CTLFLAG_RW      (CTLFLAG_RD | CTLFLAG_WR)
#define CTLFLAG_KERWR   0x40000000  /*!< Writable by a kernel request. */
#define CTLFLAG_ANYBODY 0x80000000  /*!< All users can set this var */
#define CTLFLAG_SECURE  0x02000000  /*!< Permit set only if securelevel<=0 */
#define CTLFLAG_SKIP    0x01000000  /*!< Skip this sysctl when listing */
#define CTLMASK_SECURE  0x00F00000  /*!< Secure level */
#define CTLFLAG_DYN     0x04000000  /* Dynamic oid - can be freed */
#define CTLFLAG_DYING   0x00010000  /*!< Oid is being removed */

/*
 * Secure level.   Note that CTLFLAG_SECURE == CTLFLAG_SECURE1.
 *
 * Secure when the securelevel is raised to at least N.
 */
#define        CTLSHIFT_SECURE        20
#define        CTLFLAG_SECURE1        (CTLFLAG_SECURE | (0 << CTLSHIFT_SECURE))
#define        CTLFLAG_SECURE2        (CTLFLAG_SECURE | (1 << CTLSHIFT_SECURE))
#define        CTLFLAG_SECURE3        (CTLFLAG_SECURE | (2 << CTLSHIFT_SECURE))

/**
 * Automatic OID number assignment.
 * USE THIS instead of a hardwired number from the categories below
 * to get dynamically assigned sysctl entries using the linker-set
 * technology. This is the way nearly all new sysctl variables should
 * be implemented.
 * e.g. SYSCTL_INT(_parent, OID_AUTO, name, CTLFLAG_RW, &variable, 0, "");
 */
#define OID_AUTO (-1)

/*
 * The starting number for dynamically-assigned entries.  WARNING!
 * ALL static sysctl entries should have numbers LESS than this!
 */
#define CTL_AUTO_START 0x100

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/** Arguments struct for sysctl syscall */
struct _sysctl_args {
    int * name;
    unsigned int namelen;
    void * old;
    size_t * oldlenp;
    void * new;
    size_t newlen;
};
#endif

/* User space functions. */

/**
 * Get system information by MIB style name.
 * Retrieves system information and allows processes with appropriate privileges
 * to set system information.
 * @param name      is the MIB style name list.
 * @param namelen   length of array of integers in name.
 * @param oldp      is the target buffer where old value is copied to.
 * @param oldlenp   is the length of oldp and after the call it is the length
 *                  of data copied to oldp.
 * @param newp      is set to null if no write request is intended; Otherwise
 *                  newp is set to point to a buffer that contains the new
 *                  value to be written.
 * @param newlen    is the lenght of newp or 0.
 */
int sysctl(int * name, unsigned int namelen, void * oldp, size_t * oldlenp,
           void * newp, size_t newlen);

/**
 * Lookup for a MIB node by ASCII name.
 * @param[in]  name is the ASCII representation of a MIB node.
 * @param[out] oidp is a pointer to the array where returned OID is written.
 * @param[in]  lenp is the size of the oidp array in elements.
 * @return length of oidp.
 */
int sysctlnametomib(char * name, int * oidp, int lenp);

/**
 * @param[in]  oid      is the interger name of an MIB entry.
 * @param[in]  len      is the length of the oid.
 * @param[out] strname  is the string name of the entry returned.
 * @param  strname_len  is the length of the strname, returns final length of
 *                      the string.
 * @return Same as sysctl().
 * @throws Same os sysctl().
 */
int sysctlmibtoname(int * oid, int len, char * strname, size_t * strname_len);

/**
 * Get type of MIB entry.
 * @param[in]  oid is the integer name of the entry.
 * @param[in]  len is the length of oid name.
 * @param[out] fmt is a format string of the entry, usually string
 *             representation of the type.
 * @param[out] kind is the CTL type of the entry.
 * @return 0 if succeed; Value other than zero if failed.
 * @throws Same errnos as sysctl().
 */
int sysctloidfmt(int * oid, int len, char * fmt, unsigned int * kind);

/**
 * @param[in]  oid      is the interger name of an MIB entry.
 * @param[in]  len      is the length of the oid.
 * @param[out] strname  is the string description of the entry returned.
 * @param  strname_len  is the length of the strname, returns final length of
 *                      the string.
 * @return Same as sysctl().
 * @throws Same os sysctl().
 */
int sysctlgetdesc(int * oid, int len, char * str, size_t * str_len);

/**
 * Get the next variable from MIB tree.
 * @param[in]  oid is the OID of a MIB node. Can be null;
 * @param[in]  len is the length of oid.
 * @param[out] oidn is the next oid.
 * @param[out] lenn is the length of oidn.
 * @return Same as sysctl().
 * @throws Same as sysctl().
 */
int sysctlgetnext(int * oid, int len, int * oidn, size_t * lenn);

/**
 * Test if MIBs are equal to the len.
 * @return  Returns true if MIBs are equl to the len;
 *          Returns false if MIBs differ between indexes 0..len-1.
 */
int sysctltstmib(int * left, int * right, int len);


#ifdef KERNEL_INTERNAL
#include <sys/linker_set.h>
#include <sys/types.h>
#include <sys/priv.h>

#define SYSCTL_HANDLER_ARGS \
    struct sysctl_oid * oidp, void * arg1, \
    intptr_t arg2, struct sysctl_req * req

/**
 * Sysctl request.
 * This describes the access space for a sysctl request.  This is needed
 * so that we can use the interface from the kernel or from user-space.
 */
struct sysctl_req {
    const struct cred * cred; /*!< used for access checking */
    void * oldptr;  /*!< pointer to user supplied buffer */
    size_t oldlen;  /*!< user buffer length (also returned) */
    size_t oldidx;  /*!< total data iteratively copied out */
    int (*oldfunc)(struct sysctl_req *, const void *, size_t);
    void * newptr;  /*!< buffer containing new value */
    size_t newlen;  /*!< length of the new value */
    size_t newidx;  /*!< total data iteratively copied in */
    int (*newfunc)(struct sysctl_req *, void *, size_t);
    size_t validlen;
    int flags;
};

/*
 * Sysctl flags.
 */
#define SYSCTL_REQFLAG_KERNEL 0x01 /*!< Kernel request. */

SLIST_HEAD(sysctl_oid_list, sysctl_oid);

/*
 * This describes one "oid" in the MIB tree.  Potentially more nodes can
 * be hidden behind it, expanded by the handler.
 */
struct sysctl_oid {
    struct sysctl_oid_list * oid_parent;
    SLIST_ENTRY(sysctl_oid) oid_link;
    int oid_number;
    unsigned int oid_kind;
    void * oid_arg1;
    intptr_t oid_arg2;
    const char * oid_name;
    int (*oid_handler)(SYSCTL_HANDLER_ARGS);
    const char * oid_fmt;
    int oid_refcnt;
    atomic_t oid_running;
    const char * oid_descr;
};

int sysctl_handle_bool(SYSCTL_HANDLER_ARGS);
int sysctl_handle_int(SYSCTL_HANDLER_ARGS);
int sysctl_handle_long(SYSCTL_HANDLER_ARGS);
int sysctl_handle_32(SYSCTL_HANDLER_ARGS);
int sysctl_handle_64(SYSCTL_HANDLER_ARGS);
int sysctl_handle_string(SYSCTL_HANDLER_ARGS);
int sysctl_handle_opaque(SYSCTL_HANDLER_ARGS);

/* Declare a static oid to allow child oids to be added to it. */
#define SYSCTL_DECL(name) \
    extern struct sysctl_oid_list sysctl_##name##_children

/* Hide these in macros. */
#define SYSCTL_CHILDREN(oid_ptr)                                        \
    (struct sysctl_oid_list *)(oid_ptr)->oid_arg1
#define SYSCTL_CHILDREN_SET(oid_ptr, val) (oid_ptr)->oid_arg1 = (val)
#define SYSCTL_STATIC_CHILDREN(oid_name) (&sysctl_##oid_name##_children)

#define SYSCTL_NODE_CHILDREN(parent, name) \
    sysctl_##parent##_##name##_children

/*
 * These macros provide type safety for sysctls.  SYSCTL_ALLOWED_TYPES()
 * defines a transparent union of the allowed types.  SYSCTL_ASSERT_TYPE()
 * and SYSCTL_ADD_ASSERT_TYPE() use the transparent union to assert that
 * the pointer matches the allowed types.
 *
 * The allow_0 member allows a literal 0 to be passed for ptr.
 */
#define SYSCTL_ALLOWED_TYPES(type, decls)                   \
    union sysctl_##type {                                   \
        long allow_0;                                       \
        decls                                               \
    } __attribute__((__transparent_union__));               \
                                                            \
    static inline void *                                    \
        __sysctl_assert_##type(union sysctl_##type ptr)     \
    {                                                       \
        return (ptr.a);                                     \
    }                                                       \
    struct __hack

SYSCTL_ALLOWED_TYPES(INT, int *a; );
SYSCTL_ALLOWED_TYPES(UINT, unsigned int *a; );
SYSCTL_ALLOWED_TYPES(LONG, long *a; );
SYSCTL_ALLOWED_TYPES(ULONG, unsigned long *a; );
SYSCTL_ALLOWED_TYPES(INT64, int64_t *a; long long *b; );
SYSCTL_ALLOWED_TYPES(UINT64, uint64_t *a; unsigned long long *b; );

#ifdef notyet
#define        SYSCTL_ADD_ASSERT_TYPE(type, ptr)                \
    __sysctl_assert_ ## type (ptr)
#define        SYSCTL_ASSERT_TYPE(type, ptr, parent, name)      \
    _SYSCTL_ASSERT_TYPE(type, ptr, __LINE__, parent##_##name)
#else
#define        SYSCTL_ADD_ASSERT_TYPE(type, ptr)        ptr
#define        SYSCTL_ASSERT_TYPE(type, ptr, parent, name)
#endif
#define        _SYSCTL_ASSERT_TYPE(t, p, l, id)     \
    __SYSCTL_ASSERT_TYPE(t, p, l, id)
#define __SYSCTL_ASSERT_TYPE(type, ptr, line, id)   \
    static inline void                              \
    sysctl_assert_##line##_##id(void)               \
    {                                               \
        (void)__sysctl_assert_##type(ptr);          \
    }                                               \
    struct __hack

#ifndef NO_SYSCTL_DESCR
#define        __DESCR(d) d
#else
#define        __DESCR(d) ""
#endif

/* This constructs a "raw" MIB oid. */
#define SYSCTL_OID(parent, nbr, name, kind, a1, a2, handler, fmt, descr)    \
    static struct sysctl_oid sysctl__##parent##_##name = {                  \
        .oid_parent = &sysctl_##parent##_children,                          \
        .oid_link = { NULL },                                               \
        .oid_number = nbr,                                                  \
        .oid_kind = kind,                                                   \
        .oid_arg1 = a1,                                                     \
        .oid_arg2 = a2,                                                     \
        .oid_name = #name,                                                  \
        .oid_handler = handler,                                             \
        .oid_fmt = fmt,                                                     \
        .oid_refcnt = 0,                                                    \
        .oid_running = 0,                                                   \
        .oid_descr = __DESCR(descr),                                        \
    };                                                                      \
    DATA_SET(sysctl_set, sysctl__##parent##_##name)

/**
 * This constructs a node from which other oids can hang.
 */
#define SYSCTL_NODE(parent, nbr, name, access, handler, descr)          \
    struct sysctl_oid_list SYSCTL_NODE_CHILDREN(parent, name);          \
    SYSCTL_OID(parent, nbr, name, CTLTYPE_NODE|(access),                \
    (void*)&SYSCTL_NODE_CHILDREN(parent, name), 0, handler, "N", descr)

/**
 * Oid for a string.
 * @param len can be 0 to indicate '\0' termination.
 */
#define SYSCTL_STRING(parent, nbr, name, access, arg, len, descr)   \
        SYSCTL_OID(parent, nbr, name, CTLTYPE_STRING|(access),      \
                arg, len, sysctl_handle_string, "A", descr)

/**
 * Oid for a boolean.
 */
#define SYSCTL_BOOL(parent, nbr, name, access, ptr, val, descr) \
        SYSCTL_ASSERT_TYPE(INT, ptr, parent, name);             \
        SYSCTL_OID(parent, nbr, name,                           \
            CTLTYPE_INT | (access),                             \
            ptr, val, sysctl_handle_bool, "B", descr)

/**
 * Oid for an int.
 * If ptr is NULL, val is returned.
 */
#define SYSCTL_INT(parent, nbr, name, access, ptr, val, descr)  \
        SYSCTL_ASSERT_TYPE(INT, ptr, parent, name);             \
        SYSCTL_OID(parent, nbr, name,                           \
            CTLTYPE_INT | (access),                             \
            ptr, val, sysctl_handle_int, "I", descr)

/**
 * Oid for an unsigned int.
 * If ptr is NULL, val is returned.
 */
#define SYSCTL_UINT(parent, nbr, name, access, ptr, val, descr) \
        SYSCTL_ASSERT_TYPE(UINT, ptr, parent, name);            \
        SYSCTL_OID(parent, nbr, name,                           \
            CTLTYPE_UINT | (access),                            \
            ptr, val, sysctl_handle_int, "IU", descr)

/**
 * Oid for a long.
 * The pointer must be non NULL.
 */
#define SYSCTL_LONG(parent, nbr, name, access, ptr, val, descr) \
        SYSCTL_ASSERT_TYPE(LONG, ptr, parent, name);            \
        SYSCTL_OID(parent, nbr, name,                           \
            CTLTYPE_LONG | (access),                            \
            ptr, val, sysctl_handle_long, "L", descr)

/**
 * Oid for an unsigned long.
 * The pointer must be non NULL.
 */
#define SYSCTL_ULONG(parent, nbr, name, access, ptr, val, descr)    \
        SYSCTL_ASSERT_TYPE(ULONG, ptr, parent, name);               \
        SYSCTL_OID(parent, nbr, name,                               \
            CTLTYPE_ULONG | (access),                               \
            ptr, val, sysctl_handle_long, "LU", descr)

/**
 * Oid for a 64-bit unsigned counter(9).
 * The pointer must be non NULL.
 */
#define SYSCTL_COUNTER_U64(parent, nbr, name, access, ptr, val, descr)  \
        SYSCTL_ASSERT_TYPE(UINT64, ptr, parent, name);                  \
        SYSCTL_OID(parent, nbr, name,                                   \
            CTLTYPE_U64 | (access),                                     \
            ptr, val, sysctl_handle_counter_u64, "QU", descr)

/**
 * Oid for an opaque object. Specified by a pointer and a size.
 */
#define SYSCTL_OPAQUE(parent, nbr, name, access, arg, size, fmt, descr) \
        CTASSERT(((access) & CTLTYPE) == CTLTYPE_OPAQUE);               \
        SYSCTL_OID(parent, nbr, name, CTLTYPE_STRING|(access),          \
                arg, size, sysctl_handle_opaque, fmt, descr)

/**
 * Oid for a procedure.
 * Specified by a pointer and an arg.
 * @param parent is the parent OID.
 * @param nbr OID_AUTO or predefined OID.
 * @param name is the name of the node.
 * @param access CTLTYPE and CTLFLAGs.
 */
#define SYSCTL_PROC(parent, nbr, name, access, ptr, arg, handler, fmt, descr)  \
    CTASSERT(((access) & CTLTYPE) != 0);                                       \
    SYSCTL_OID(parent, nbr, name, (access),                                    \
        ptr, arg, handler, fmt, descr)

/*
 * A macro to generate a read-only sysctl to indicate the presense of optional
 * kernel features.
 */
#define FEATURE(name, desc)                                                     \
        SYSCTL_INT(_kern_features, OID_AUTO, name, CTLFLAG_RD,                  \
            NULL, 1, desc)

#endif /* KERNEL_INTERNAL */

/*
 * Top-level identifiers
 */
#define CTL_UNSPEC              0   /* unused */
#define CTL_KERN                1   /* "high kernel": proc, limits */
#define CTL_VM                  2   /* virtual memory */
#define CTL_VFS                 3   /* filesystem, mount type is next */
#define CTL_NET                 4   /* network, see socket.h */
#define CTL_DEBUG               5   /* debugging parameters */
#define CTL_HW                  6   /* generic cpu/io */
#define CTL_MACHDEP             7   /* machine dependent */
#define CTL_MAXID               8   /* number of valid top-level ids */

/*
 * Identifiers here are mostly compatible with BSD but we don't support structs
 * so those identifiers are left out.
 * RFE Many of these identifiers are not yet actually mapped but these are left
 * here as a reservations for future use.
 */

/*
 * _sysctl
 */
#define _CTLMAGIC_NAME          1   /* Get the name of a MIB variable. */
#define _CTLMAGIC_NEXT          2   /* Get the next variable from MIB tree. */
#define _CTLMAGIC_NAME2OID      3   /* String name to integer name of
                                     * the variable. */
#define _CTLMAGIC_OIDFMT        4   /* Get format and type of a MIB variable. */
#define _CTLMAGIC_OIDDESCR      5   /* Get description string of a MIB
                                     * variable. */

/*
 * CTL_KERN identifiers
 */
#define KERN_OSTYPE             1   /*!< string: system version */
#define KERN_OSRELEASE          2   /*!< string: system release */
#define KERN_OSREV              3   /*!< int: system revision */
#define KERN_VERSION            4   /*!< string: compile time info */
#define KERN_MAXPROC            5   /*!< int: max processes */
#define KERN_DFLSIZ             6   /*!< int: default main() stack size */
#define KERN_MAXSIZ             7   /*!< int: max main() stack size */
#define KERN_MAXFILES           8   /*!< int: max open files */
#define KERN_ARGMAX             9   /*!< int: max arguments to exec */
#define KERN_HOSTNAME           10  /*!< string: hostname */
#define KERN_PROC               11  /*!< node: process entries */
#define KERN_PROF               12  /*!< node: kernel profiling info */
#define KERN_POSIX1             13  /*!< int: POSIX.1 version */
#define KERN_OSRELDATE          14  /*!< int: kernel release date */
#define KERN_NTP_PLL            15  /*!< node: NTP PLL control */
#define KERN_BOOTFILE           16  /*!< string: name of booted kernel */
#define KERN_MAXFILESPERPROC    17  /*!< int: max open files per proc */
#define KERN_IPC                18  /*!< node: anything related to IPC */
#define KERN_LOGSIGEXIT         19  /*!< int: do we log sigexit procs? */
#define KERN_HOSTUUID           20  /*!< string: host UUID identifier */

/*
 * KERN_PROC subtypes
 */
#define KERN_PROC_PID           1   /*!< Get proc data by process id */
#define KERN_PROC_PGRP          2   /*!< Get process group info */
#define KERN_PROC_SESSION       3   /*!< Get session info */

/*
 * KERN_PROC_PID subtypes
 */
#define KERN_PROC_PSTAT         1   /*!< Get process stats */
#define KERN_PROC_VMMAP         2   /*!< VM map entries for process */
#define KERN_PROC_FILEDESC      3   /*!< File descriptors for process */
#define KERN_PROC_NFDS          4   /*!< number of open file descriptors */
#define KERN_PROC_GROUPS        5   /*!< process groups */
#define KERN_PROC_ENV           6   /*!< get environment */
#define KERN_PROC_ARGS          7   /*!< get/set arguments/proctitle */
#define KERN_PROC_RLIMIT        8   /*!< process resource limits */
#define KERN_PROC_SIGTRAMP      9   /*!< signal trampoline location */
#define KERN_PROC_CWD           10  /*!< process current working directory */

/*
 * KERN_IPC identifiers
 */
#define KIPC_MAXSOCKBUF         1   /*!< int: max size of a socket buffer */
#define KIPC_SOCKBUF_WASTE      2   /*!< int: wastage factor in sockbuf */
#define KIPC_SOMAXCONN          3   /*!< int: max length of connection q */
#define KIPC_MAX_LINKHDR        4   /*!< int: max length of link header */
#define KIPC_MAX_PROTOHDR       5   /*!< int: max length of network header */
#define KIPC_MAX_HDR            6   /*!< int: max total length of headers */
#define KIPC_MAX_DATALEN        7   /*!< int: max length of data? */

/*
 * CTL_HW identifiers
 */
#define HW_MACHINE              1   /*!< string: machine class */
#define HW_MODEL                2   /*!< string: specific machine model */
#define HW_NCPU                 3   /*!< int: number of cpus */
#define HW_BYTEORDER            4   /*!< int: machine byte order */
#define HW_PHYSMEM_START        5   /*!< int: phys mem start */
#define HW_PHYSMEM              6   /*!< int: total memory */
#define HW_USERMEM              7   /*!< int: non-kernel memory */
#define HW_PAGESIZE             8   /*!< int: software page size */
#define HW_FLOATINGPT           9   /*!< int: has HW floating point? */
#define HW_MACHINE_ARCH         10  /*!< string: machine architecture */
#define HW_REALMEM              11  /*!< int: 'real' memory */

#ifdef KERNEL_INTERNAL

/*
 * Declare oids.
 */
extern struct sysctl_oid_list sysctl__children;

SYSCTL_DECL(_kern);
SYSCTL_DECL(_kern_proc);
SYSCTL_DECL(_vm);
SYSCTL_DECL(_vfs);
SYSCTL_DECL(_debug);
SYSCTL_DECL(_hw);
SYSCTL_DECL(_hw_pm);
SYSCTL_DECL(_machdep);
SYSCTL_DECL(_security);

void sysctl_register_oid(struct sysctl_oid * oidp);
void sysctl_unregister_oid(struct sysctl_oid * oidp);
int sysctl_find_oid(int * name, unsigned int namelen, struct sysctl_oid ** noid,
        int * nindx, struct sysctl_req * req);

/* Dynamic */

/**
 * Remove dynamically created sysctl trees.
 * @param oidp      top of the tree to be removed
 * @param del       if 0 - just deregister, otherwise free up entries as well
 * @param recurse   if != 0 traverse the subtree to be deleted
 */
int sysctl_remove_oid(struct sysctl_oid * oidp, int del, int recurse);

/**
 * Create new sysctls at run time.
 */
struct sysctl_oid * sysctl_add_oid(struct sysctl_oid_list * parent,
                                   const char * name, int kind,
                                   void * arg1, intmax_t arg2,
                                   int (*handler)(SYSCTL_HANDLER_ARGS),
                                   const char * fmt, const char * descr);

/**
 * Rename an existing oid.
 */
int sysctl_rename_oid(struct sysctl_oid * oidp, const char * name);

/*
 * Reparent an existing oid.
 */
int sysctl_move_oid(struct sysctl_oid * oid, struct sysctl_oid_list * parent);

/* End of dynamic */

/*
 * In-kernel sysctl by name.
 * @aram cred if NULL the kernel's credentials are used.
 */
int kernel_sysctlbyname(struct cred * cred, char * name, void * old,
                        size_t * oldlenp, void * new, size_t newlen,
                        size_t * retval, int flags);
/*
 * In-kernel sysctl.
 * @aram cred if NULL the kernel's credentials are used.
 */
int kernel_sysctl(struct cred * cred, int * name, unsigned int namelen,
                  void * old, size_t * oldlenp, void * new, size_t newlen,
                  size_t * retval, int flags);

int kernel_sysctl_read(int * name, unsigned int namelen,
                       void * old, size_t oldlen);

int kernel_sysctl_write(int * name, unsigned int namelen,
                        const void * new, size_t newlen);

#endif /* KERNEL_INTERNAL */

#endif /* _SYS_SYSCTL_H_ */
