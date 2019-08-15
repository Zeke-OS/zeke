/**
 *******************************************************************************
 * @file    priv.h
 * @author  Olli Vanhoja
 * @brief   User credentials.
 * @section LICENSE
 * Copyright (c) 2019 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2014, 2015, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2006 nCircle Network Security, Inc.
 * All rights reserved.
 *
 * This software was developed by Robert N. M. Watson for the TrustedBSD
 * Project under contract to nCircle Network Security, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR, NCIRCLE NETWORK SECURITY,
 * INC., OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @addtogroup LIBC
 * @{
 */

/**
 * @addtogroup priv
 * @{
 */

#ifndef _SYS_PRIV_H_
#define _SYS_PRIV_H_

/*
 * Privilege list, sorted loosely by kernel subsystem.
 *
 * Think carefully before adding or reusing one of these privileges -- are
 * there existing instances referring to the same privilege?
 * Particular numeric privilege assignments are part of the
 * kernel API, and should not be changed across minor releases.
 *
 * The remaining privileges typically correspond to one or a small
 * number of specific privilege checks, and have (relatively) precise
 * meanings.  They are loosely sorted into a set of base system
 * privileges, such as the ability to reboot, and then loosely by
 * subsystem, indicated by a subsystem name.
 */

#define PRIV_FOREACH_CAP(CAP) \
    /*
     * Privileges associated with the security framework.
     */ \
    CAP(PRIV_CLRCAP,         1) /* Can clear process capabilities. */ \
    CAP(PRIV_SETEFF,         2) /* Can set effective capabilities. */ \
    CAP(PRIV_SETBND,         3) /* Can set bounding capabilities. */ \
    CAP(PRIV_EXEC_B2E,       4) /* Copy bnd set to eff set on exec. */ \
    /* */ \
    CAP(PRIV_ACCT,          10) /* Manage process accounting. */ \
    CAP(PRIV_MAXFILES,      11) /* Exceed system open files limit. */ \
    CAP(PRIV_MAXPROC,       12) /* Exceed system processes limit. */ \
    CAP(PRIV_KTRACE,        13) /* Set/clear KTRFAC_ROOT on ktrace. */ \
    CAP(PRIV_SETDUMPER,     14) /* Configure dump device. */ \
    CAP(PRIV_REBOOT,        15) /* Can reboot system. */ \
    CAP(PRIV_SWAPON,        16) /* Can swapon(). */ \
    CAP(PRIV_SWAPOFF,       17) /* Can swapoff(). */ \
    CAP(PRIV_MSGBUF,        18) /* Can read kernel message buffer. */ \
    CAP(PRIV_IO,            19) /* Can perform low-level I/O. */ \
    CAP(PRIV_KEYBOARD,      20) /* Reprogram keyboard. */ \
    CAP(PRIV_DRIVER,        21) /* Low-level driver privilege. */ \
    CAP(PRIV_ADJTIME,       22) /* Set time adjustment. */ \
    CAP(PRIV_NTP_ADJTIME,   23) /* Set NTP time adjustment. */ \
    CAP(PRIV_CLOCK_SETTIME, 24) /* Can call clock_settime. */ \
    CAP(PRIV_SETTIMEOFDAY,  25) /* Can call settimeofday. */ \
    /*
     * Credential management privileges.
     */ \
    CAP(PRIV_CRED_SETUID,   30) /* setuid. */ \
    CAP(PRIV_CRED_SETEUID,  31) /* seteuid to !ruid and !svuid. */ \
    CAP(PRIV_CRED_SETSUID,  32) \
    CAP(PRIV_CRED_SETGID,   33) /* setgid. */ \
    CAP(PRIV_CRED_SETEGID,  34) /* setgid to !rgid and !svgid. */ \
    CAP(PRIV_CRED_SETSGID,  35) \
    CAP(PRIV_CRED_SETGROUPS, 36) /* Set process additional groups. */ \
    /*
     * Kernel and hardware manipulation.
     */ \
    CAP(PRIV_KLD_LOAD,      40) /* Load a kernel module. */ \
    CAP(PRIV_KLD_UNLOAD,    41) /* Unload a kernel module. */ \
    CAP(PRIV_KMEM_READ,     42) /* Open mem/kmem for reading. */ \
    CAP(PRIV_KMEM_WRITE,    43) /* Open mem/kmem for writing. */ \
    CAP(PRIV_FIRMWARE_LOAD, 44) /* Can load firmware. */ \
    CAP(PRIV_CPUCTL_WRMSR,  45) /* Write model-specific register. */ \
    CAP(PRIV_CPUCTL_UPDATE, 46) /* Update cpu microcode. */ \
    /*
     * Process-related privileges.
     */ \
    CAP(PRIV_PROC_FORK,     60) /* Can fork() */ \
    CAP(PRIV_PROC_LIMIT,    61) /* Exceed user process limit. */ \
    CAP(PRIV_PROC_SETLOGIN, 62) /* Can call setlogin. */ \
    CAP(PRIV_PROC_SETRLIMIT, 63) /* Can raise resources limits. */ \
    CAP(PRIV_PROC_STAT,     64) /* Can get status info of any process. */ \
    /*
     * Scheduling privileges.
     */ \
    CAP(PRIV_SCHED_DIFFCRED, 70) /* Exempt scheduling other users. */ \
    CAP(PRIV_SCHED_SETPRIORITY, 71) /* Can set lower nice value for proc. */ \
    CAP(PRIV_SCHED_RTPRIO,  72) /* Can set real time scheduling. */ \
    CAP(PRIV_SCHED_SETPOLICY, 73) /* Can set scheduler policy. */ \
    CAP(PRIV_SCHED_SET,     74) /* Can set thread scheduler. */ \
    CAP(PRIV_SCHED_SETPARAM, 75)  /* Can set thread scheduler params. */ \
    /*
     * IPC
     * - Signal privileges.
     * - System V IPC privileges.
     * - POSIX message queue privileges.
     * - POSIX semaphore privileges.
     */ \
    CAP(PRIV_SIGNAL_OTHER,  80) /* Exempt signalling other users. */ \
    CAP(PRIV_SIGNAL_ACTION, 81) /* Change signal actions. */ \
    CAP(PRIV_IPC_READ,      82) /* Can override IPC read perm. */ \
    CAP(PRIV_IPC_WRITE,     83) /* Can override IPC write perm. */ \
    CAP(PRIV_IPC_ADMIN,     84) /* Can override IPC owner-only perm. */ \
    CAP(PRIV_IPC_MSGSIZE,   85) /* Exempt IPC message queue limit. */ \
    CAP(PRIV_MQ_ADMIN,      86) /* Can override msgq owner-only perm. */ \
    CAP(PRIV_SEM_WRITE,     87) /* Can override sem write perm. */ \
    /*
     * Sysctl privileges.
     */ \
    CAP(PRIV_SYSCTL_DEBUG,  90) /* Can invoke sysctl.debug. */ \
    CAP(PRIV_SYSCTL_WRITE,  91) /* Can write sysctls. */ \
    /*
     * TTY privileges.
     */ \
    CAP(PRIV_TTY_CONSOLE,   100) /* Set console to tty. */ \
    CAP(PRIV_TTY_DRAINWAIT, 101) /* Set tty drain wait time. */ \
    CAP(PRIV_TTY_DTRWAIT,   102) /* Set DTR wait on tty. */ \
    CAP(PRIV_TTY_EXCLUSIVE, 103) /* Override tty exclusive flag. */ \
    CAP(PRIV_TTY_STI,       105) /* Simulate input on another tty. */ \
    CAP(PRIV_TTY_SETA,      106) /* Set tty termios structure. */ \
    /*
     * VFS privileges.
     */ \
    CAP(PRIV_VFS_ADMIN,     110) /* vnode admin perm. Override any DAC. */ \
    CAP(PRIV_VFS_READ,      111) /* open file for read. */ \
    CAP(PRIV_VFS_WRITE,     112) /* open file for write. */ \
    CAP(PRIV_VFS_WRITE_SYS, 113) /* open system file for write. */ \
    CAP(PRIV_VFS_EXEC,      114) /* vnode exec perm. */ \
    CAP(PRIV_VFS_LOOKUP,    115) /* vnode lookup perm. */ \
    CAP(PRIV_VFS_CHOWN,     116) /* Can set user; group to non-member. */ \
    CAP(PRIV_VFS_CHROOT,    117) /* chroot(). */ \
    CAP(PRIV_VFS_RETAINSUGID, 118) /* Can retain sugid bits on change. */ \
    CAP(PRIV_VFS_LINK,      119) /* bsd.hardlink_check_uid */ \
    CAP(PRIV_VFS_SETGID,    120) /* Can setgid if not in group. */ \
    CAP(PRIV_VFS_STICKYFILE, 121) /* Can set sticky bit on file. */ \
    CAP(PRIV_VFS_SYSFLAGS,  122) /* Can modify system flags. */ \
    CAP(PRIV_VFS_UNMOUNT,   123) /* Can unmount(). */ \
    CAP(PRIV_VFS_STAT,      124) /* Stat perm. */ \
    CAP(PRIV_VFS_MOUNT,     125) /* Can mount(). */ \
    CAP(PRIV_VFS_MOUNT_OWNER, 126) /* Can manage other users' file systems. */ \
    CAP(PRIV_VFS_MOUNT_PERM,    127) /* Override dev node perms at mount. */ \
    CAP(PRIV_VFS_MOUNT_SUIDDIR, 128) /* Can set MNT_SUIDDIR on mount. */ \
    CAP(PRIV_VFS_MOUNT_NONUSER, 129) /* Can perform a non-user mount. */ \
    /*
     * Virtual memory privileges.
     */ \
    CAP(PRIV_VM_PROT_EXEC,  140) /* Can set a memory region executable. */ \
    CAP(PRIV_VM_MADV_PROTECT, 141) /* Can set MADV_PROTECT. */ \
    CAP(PRIV_VM_MLOCK,      142) /* Can mlock(), mlockall(). */ \
    CAP(PRIV_VM_MUNLOCK,    143) /* Can munlock(), munlockall(). */ \
    /*
     * Network stack privileges.
     */ \
    CAP(PRIV_NET_BRIDGE,    150) /* Administer bridge. */ \
    CAP(PRIV_NET_GRE,       151) /* Administer GRE. */ \
    CAP(PRIV_NET_BPF,       152) /* Monitor BPF. */ \
    CAP(PRIV_NET_RAW,       153) /* Open raw socket. */ \
    CAP(PRIV_NET_ROUTE,     154) /* Administer routing. */ \
    CAP(PRIV_NET_TAP,       155) /* Can open tap device. */ \
    CAP(PRIV_NET_SETIFMTU,  156) /* Set interface MTU. */ \
    CAP(PRIV_NET_SETIFFLAGS, 157) /* Set interface flags. */ \
    CAP(PRIV_NET_SETIFCAP,  158) /* Set interface capabilities. */ \
    CAP(PRIV_NET_SETIFNAME, 159) /* Set interface name. */ \
    CAP(PRIV_NET_SETIFMETRIC, 160) /* Set interface metrics. */ \
    CAP(PRIV_NET_SETIFPHYS, 161) /* Set interface physical layer prop. */ \
    CAP(PRIV_NET_SETIFMAC,  162) /* Set interface MAC label. */ \
    CAP(PRIV_NET_ADDMULTI,  163) /* Add multicast addr. to ifnet. */ \
    CAP(PRIV_NET_DELMULTI,  164) /* Delete multicast addr. from ifnet. */ \
    CAP(PRIV_NET_HWIOCTL,   165) /* Issue hardware ioctl on ifnet. */ \
    CAP(PRIV_NET_SETLLADDR, 166) /* Set interface link-level address. */ \
    CAP(PRIV_NET_ADDIFGROUP, 167) /* Add new interface group. */ \
    CAP(PRIV_NET_DELIFGROUP, 168) /* Delete interface group. */ \
    CAP(PRIV_NET_IFCREATE,  169) /* Create cloned interface. */ \
    CAP(PRIV_NET_IFDESTROY, 170) /* Destroy cloned interface. */ \
    CAP(PRIV_NET_ADDIFADDR, 171) /* Add protocol addr to interface. */ \
    CAP(PRIV_NET_DELIFADDR, 172) /* Delete protocol addr on interface. */ \
    CAP(PRIV_NET_LAGG,      173) /* Administer lagg interface. */ \
    CAP(PRIV_NET_GIF,       174) /* Administer gif interface. */ \
    CAP(PRIV_NET_SETIFVNET, 175) /* Move interface to vnet. */ \
    CAP(PRIV_NET_SETIFDESCR, 176) /* Set interface description. */ \
    CAP(PRIV_NET_SETIFFIB,  177) /* Set interface fib. */ \
    /*
     * IPv4 and IPv6 privileges.
     */ \
    CAP(PRIV_NETINET_RESERVEDPORT, 180) /* Bind low port number. */ \
    CAP(PRIV_NETINET_IPFW,  181) /* Administer IPFW firewall. */ \
    CAP(PRIV_NETINET_DIVERT, 182) /* Open IP divert socket. */ \
    CAP(PRIV_NETINET_PF,    183) /* Administer pf firewall. */ \
    CAP(PRIV_NETINET_DUMMYNET, 184) /* Administer DUMMYNET. */ \
    CAP(PRIV_NETINET_CARP,  185) /* Administer CARP. */ \
    CAP(PRIV_NETINET_MROUTE, 186) /* Administer multicast routing. */ \
    CAP(PRIV_NETINET_RAW,   187) /* Open netinet raw socket. */ \
    CAP(PRIV_NETINET_GETCRED, 188) /* Query netinet pcb credentials. */ \
    CAP(PRIV_NETINET_ADDRCTRL6, 189) /* Administer IPv6 address scopes. */ \
    CAP(PRIV_NETINET_ND6,   190) /* Administer IPv6 neighbor disc. */ \
    CAP(PRIV_NETINET_SCOPE6, 191) /* Administer IPv6 address scopes. */ \
    CAP(PRIV_NETINET_ALIFETIME6, 192) /* Administer IPv6 address lifetimes. */ \
    CAP(PRIV_NETINET_IPSEC,  193) /* Administer IPSEC. */ \
    CAP(PRIV_NETINET_REUSEPORT, 194) /* Allow [rapid] port/address reuse. */ \
    CAP(PRIV_NETINET_SETHDROPTS, 195) /* Set certain IPv4/6 header options. */ \
    CAP(PRIV_NETINET_BINDANY, 196) /* Allow bind to any address. */ \

#define PRIV_GENERATE_CAP_ENUM(NAME, NUM) NAME = NUM,
#define PRIV_GENERATE_CAP_STRING_ARRAY(NAME, NUM) [NUM] = #NAME,

/*
 * Priv bitmap size.
 */
#define _PRIV_MENT 256
/* Can't use E2BITMAP_SIZE here because this is exposed to the userland. */
#define _PRIV_MLEN (_PRIV_MENT / (4 * 8))
/* Size in bytes */
#define _PRIV_MSIZE (_PRIV_MLEN * 4)

enum priv_capability {
    PRIV_FOREACH_CAP(PRIV_GENERATE_CAP_ENUM)
};

const char * priv_cap_name[_PRIV_MENT];

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/**
 * Arguments struct for SYSCALL_PROC_CRED
 * Set value to -1 for get only.
 */
struct _proc_credctl_args {
    uid_t ruid;
    uid_t euid;
    uid_t suid;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;
};

/**
 * Modes for SYSCALL_PRIV_PCAP syscall.
 */
enum priv_pcap_mode {
    PRIV_PCAP_MODE_GET_EFF = 0, /*!< Get the status of an effective cap. */
    PRIV_PCAP_MODE_SET_EFF = 1, /*!< Set an effective cap. */
    PRIV_PCAP_MODE_CLR_EFF = 2, /*!< Clear an effective cap. */
    PRIV_PCAP_MODE_GET_BND = 3, /*!< Get the status of a bounding cap. */
    PRIV_PCAP_MODE_SET_BND = 4, /*!< Set a bounding cap. */
    PRIV_PCAP_MODE_CLR_BND = 5, /*!< Clear a bounding cap. */
    PRIV_PCAP_MODE_RST_BND = 6, /*!< Reset the bounding caps to the default. */
};

/**
 * Argument struct for SYSCALL_PRIV_PCAP
 */
struct _priv_pcap_args {
    enum priv_pcap_mode mode;
    size_t priv;
};

/**
 * Argument struct for SYSCALL_PRIV_PCAP_GETALL
 */
struct _priv_pcap_getall_args {
    uint32_t * effective;
    uint32_t * bounding;
};
#endif


#ifdef KERNEL_INTERNAL

#include <sys/param.h>
#include <bitmap.h>

struct cred {
    uid_t uid, euid, suid;
    gid_t gid, egid, sgid;
    gid_t sup_gid[NGROUPS_MAX];

    /**
     * Effective capabilities set.
     * These are the capabilities that are currently effective fro the process.
     * New capabilities can be added given that the capability to be added is
     * also set in the bounding capabilities set and the process has
     * PRIV_SETEFF set in the effective capabilities set.
     * A process can always remove effective capabilities from itself given
     * that PRIV_CLRCAP is set in the effective capabilities.
     */
    bitmap_t pcap_effmap[_PRIV_MLEN];

    /**
     * Bounding capabilities set.
     * These are the capabitlities that can be set if the process has a
     * privilege to set capabilities. A process can always remove bounding
     * if PRIV_CLRCAP is set in the effective capabilities.
     * New bounding capabilities can be added only if PRIV_SETBND is set in the
     * effective capabilities.
     */
    bitmap_t pcap_bndmap[_PRIV_MLEN];
};

/**
 * Test active securelevel.
 * Test whether or not the active security level is greater than or equal to
 * the supplied level.
 * @param level is the needed security level.
 * @return  Returns -EPERM if condition evaluated to true; Otherwise zero.
 */
int securelevel_ge(int level);

/**
 * Test active securelevel.
 * Test whether or not the active security level is greater than the
 * supplied level.
 * @param level is the needed security level.
 * @return  Returns -EPERM if condition evaluated to true; Otherwise zero.
 */
int securelevel_gt(int level);

/**
 * @addtogroup priv_check
 * The priv interfaces check to see if specific system privileges are granted
 * to the passed process.
 *
 * Privileges are typically granted based on one of two base system policies:
 * the superuser policy, which grants privilege based on the effective
 * (or sometimes read) UID having a value of 0 and process capability maps.
 * @{
 */

int priv_grp_is_member(const struct cred * cred, gid_t gid);

int priv_cred_eff_get(const struct cred * cred, int priv);
int priv_cred_eff_set(struct cred * cred, int priv);
int priv_cred_eff_clear(struct cred * cred, int priv);
int priv_cred_bound_get(const struct cred * cred, int priv);
int priv_cred_bound_set(struct cred * cred, int priv);
int priv_cred_bound_clear(struct cred * cred, int priv);

/**
 * Initialize a cred struct.
 * @param cred is a pointer to an uninitialized cred struct.
 */
void priv_cred_init(struct cred * cred);

/**
 * Initialize credentials inherited on fork.
 * 1. UIDs and GIDs are inherited as is
 * 2. Effective capabilities are inherited as is except for capabilities that
 *    are no longer set in the bounding capabilities set
 * 3. Bounding capabilities set is inherited as is
 * @param cred is the destination credential.
 */
void priv_cred_init_fork(struct cred * cred);

/**
 * Init credentials after exec.
 */
void priv_cred_init_exec(struct cred * cred);

/**
 * Check privileges.
 * @param proc is a pointer to a process.
 * @param priv is a PRIV.
 * @return Typically, 0 will be returned for success, and -EPERM will be
 *         returned on failure. In case of invalid arguments -EINVAL is
 *         returned.
 */
int priv_check(const struct cred * cred, int priv);

/**
 * Check credentials to change the state of an object protected by tocred.
 * @param fromcred is the accessing credential.
 * @param tocred is the target.
 * @param priv is a PRIV.
 * @return Typically, 0 will be returned for success, and -EPERM will be
 *         returned on failure. In case of invalid arguments -EINVAL is
 *         returned.
 */
int priv_check_cred(const struct cred * fromcred, const struct cred * tocred,
                    int priv);

/**
 * @}
 */

#else
int priv_setpcap(int bounding, size_t priv, int value);
int priv_getpcap(int bounding, size_t priv);
int priv_rstpcap(void);
int priv_getpcaps(uint32_t * effective, uint32_t * bounding);
#endif

#endif /* !_SYS_PRIV_H_ */

/**
 * @}
 */

/**
 * @}
 */
