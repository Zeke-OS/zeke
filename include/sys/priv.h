/**
 *******************************************************************************
 * @file    priv.h
 * @author  Olli Vanhoja
 * @brief   User credentials.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * there existing instances referring to the same privilege?  Third party
 * vendors may request the assignment of privileges to be used in loadable
 * modules.  Particular numeric privilege assignments are part of the
 * loadable kernel module ABI, and should not be changed across minor
 * releases.
 *
 * When adding a new privilege, remember to determine if it's appropriate for
 * use in jail, and update the privilege switch in kern_jail.c as necessary.
 */

/*
 * The remaining privileges typically correspond to one or a small
 * number of specific privilege checks, and have (relatively) precise
 * meanings.  They are loosely sorted into a set of base system
 * privileges, such as the ability to reboot, and then loosely by
 * subsystem, indicated by a subsystem name.
 */
#define PRIV_ACCT            1  /* Manage process accounting. */
#define PRIV_MAXFILES        2  /* Exceed system open files limit. */
#define PRIV_MAXPROC         3  /* Exceed system processes limit. */
#define PRIV_KTRACE          4  /* Set/clear KTRFAC_ROOT on ktrace. */
#define PRIV_SETDUMPER       5  /* Configure dump device. */
#define PRIV_REBOOT          6  /* Can reboot system. */
#define PRIV_SWAPON          7  /* Can swapon(). */
#define PRIV_SWAPOFF         8  /* Can swapoff(). */
#define PRIV_MSGBUF          9  /* Can read kernel message buffer. */
#define PRIV_IO             10  /* Can perform low-level I/O. */
#define PRIV_KEYBOARD       11  /* Reprogram keyboard. */
#define PRIV_DRIVER         12  /* Low-level driver privilege. */
#define PRIV_ADJTIME        13  /* Set time adjustment. */
#define PRIV_NTP_ADJTIME    14  /* Set NTP time adjustment. */
#define PRIV_CLOCK_SETTIME  15  /* Can call clock_settime. */
#define PRIV_SETTIMEOFDAY   16  /* Can call settimeofday. */

/*
 * Credential management privileges.
 */
#define PRIV_CRED_SETUID    20  /* setuid. */
#define PRIV_CRED_SETEUID   21  /* seteuid to !ruid and !svuid. */
#define PRIV_CRED_SETSUID   22
#define PRIV_CRED_SETGID    23  /* setgid. */
#define PRIV_CRED_SETEGID   24  /* setgid to !rgid and !svgid. */
#define PRIV_CRED_SETSGID   25
#define PRIV_CRED_SETGROUPS 26  /* Set process additional groups. */

/*
 * Kernel and hardware manipulation.
 */
#define PRIV_KLD_LOAD       40  /* Load a kernel module. */
#define PRIV_KLD_UNLOAD     41  /* Unload a kernel module. */
#define PRIV_KMEM_READ      42  /* Open mem/kmem for reading. */
#define PRIV_KMEM_WRITE     43  /* Open mem/kmem for writing. */
#define PRIV_FIRMWARE_LOAD  44  /* Can load firmware. */
#define PRIV_CPUCTL_WRMSR   45  /* Write model-specific register. */
 #define PRIV_CPUCTL_UPDATE 46  /* Update cpu microcode. */

/*
 * Privileges associated with the security framework.
 */
#define PRIV_ALTPCAP        50 /* Privilege to change process capabilities. */

/*
 * Process-related privileges.
 */
#define PRIV_PROC_LIMIT     60 /* Exceed user process limit. */
#define PRIV_PROC_SETLOGIN  61 /* Can call setlogin. */
#define PRIV_PROC_SETRLIMIT 62 /* Can raise resources limits. */

/*
 * IPC
 * - Signal privileges.
 * - System V IPC privileges.
 * - POSIX message queue privileges.
 * - POSIX semaphore privileges.
 */
#define PRIV_SIGNAL_OTHER   100 /* Exempt signalling other users. */
#define PRIV_SIGNAL_ACTION  101 /* Change signal actions. */
#define PRIV_IPC_READ       70  /* Can override IPC read perm. */
#define PRIV_IPC_WRITE      71  /* Can override IPC write perm. */
#define PRIV_IPC_ADMIN      72  /* Can override IPC owner-only perm. */
#define PRIV_IPC_MSGSIZE    73  /* Exempt IPC message queue limit. */
#define PRIV_MQ_ADMIN       74  /* Can override msgq owner-only perm. */
#define PRIV_SEM_WRITE      75  /* Can override sem write perm. */

/*
 * Scheduling privileges.
 */
#define PRIV_SCHED_DIFFCRED 80  /* Exempt scheduling other users. */
#define PRIV_SCHED_SETPRIORITY 81 /* Can set lower nice value for proc. */
#define PRIV_SCHED_RTPRIO   82  /* Can set real time scheduling. */
#define PRIV_SCHED_SETPOLICY 83 /* Can set scheduler policy. */
#define PRIV_SCHED_SET      84  /* Can set thread scheduler. */
#define PRIV_SCHED_SETPARAM 85  /* Can set thread scheduler params. */

/*
 * Sysctl privileges.
 */
#define PRIV_SYSCTL_DEBUG   90  /* Can invoke sysctl.debug. */
#define PRIV_SYSCTL_WRITE   91  /* Can write sysctls. */

/*
 * TTY privileges.
 */
#define PRIV_TTY_CONSOLE    100 /* Set console to tty. */
#define PRIV_TTY_DRAINWAIT  101 /* Set tty drain wait time. */
#define PRIV_TTY_DTRWAIT    102 /* Set DTR wait on tty. */
#define PRIV_TTY_EXCLUSIVE  103 /* Override tty exclusive flag. */
#define PRIV_TTY_STI        105 /* Simulate input on another tty. */
#define PRIV_TTY_SETA       106 /* Set tty termios structure. */

/*
 * VFS privileges.
 */
#define PRIV_VFS_READ       110 /* vnode read perm. */
#define PRIV_VFS_WRITE      111 /* vnode write perm. */
#define PRIV_VFS_ADMIN      112 /* vnode admin perm. */
#define PRIV_VFS_EXEC       113 /* vnode exec perm. */
#define PRIV_VFS_LOOKUP     114 /* vnode lookup perm. */
#define PRIV_VFS_CHOWN      115 /* Can set user; group to non-member. */
#define PRIV_VFS_CHROOT     116 /* chroot(). */
#define PRIV_VFS_RETAINSUGID 117 /* Can retain sugid bits on change. */
#define PRIV_VFS_LINK       118 /* bsd.hardlink_check_uid */
#define PRIV_VFS_SETGID     119 /* Can setgid if not in group. */
#define PRIV_VFS_STICKYFILE 120 /* Can set sticky bit on file. */
#define PRIV_VFS_SYSFLAGS   121 /* Can modify system flags. */
#define PRIV_VFS_UNMOUNT    122 /* Can unmount(). */
#define PRIV_VFS_STAT       123 /* Stat perm. */
#define PRIV_VFS_MOUNT      124 /* Can mount(). */
#define PRIV_VFS_MOUNT_OWNER    125 /* Can manage other users' file systems. */
#define PRIV_VFS_MOUNT_EXPORTED 126 /* Can set MNT_EXPORTED on mount. */
#define PRIV_VFS_MOUNT_PERM     127 /* Override dev node perms at mount. */
#define PRIV_VFS_MOUNT_SUIDDIR  128 /* Can set MNT_SUIDDIR on mount. */
#define PRIV_VFS_MOUNT_NONUSER  129 /* Can perform a non-user mount. */


/*
 * Virtual memory privileges.
 */
#define PRIV_VM_PROT_EXEC    130 /* Can set a memory region executable. */
#define PRIV_VM_MADV_PROTECT 131 /* Can set MADV_PROTECT. */
#define PRIV_VM_MLOCK        132 /* Can mlock(), mlockall(). */
#define PRIV_VM_MUNLOCK      133 /* Can munlock(), munlockall(). */

/*
 * Network stack privileges.
 */
#define PRIV_NET_BRIDGE     140 /* Administer bridge. */
#define PRIV_NET_GRE        141 /* Administer GRE. */
#define PRIV_NET_BPF        142 /* Monitor BPF. */
#define PRIV_NET_RAW        143 /* Open raw socket. */
#define PRIV_NET_ROUTE      144 /* Administer routing. */
#define PRIV_NET_TAP        145 /* Can open tap device. */
#define PRIV_NET_SETIFMTU   146 /* Set interface MTU. */
#define PRIV_NET_SETIFFLAGS 147 /* Set interface flags. */
#define PRIV_NET_SETIFCAP   148 /* Set interface capabilities. */
#define PRIV_NET_SETIFNAME  149 /* Set interface name. */
#define PRIV_NET_SETIFMETRIC 150 /* Set interface metrics. */
#define PRIV_NET_SETIFPHYS  151 /* Set interface physical layer prop. */
#define PRIV_NET_SETIFMAC   152 /* Set interface MAC label. */
#define PRIV_NET_ADDMULTI   153 /* Add multicast addr. to ifnet. */
#define PRIV_NET_DELMULTI   154 /* Delete multicast addr. from ifnet. */
#define PRIV_NET_HWIOCTL    155 /* Issue hardware ioctl on ifnet. */
#define PRIV_NET_SETLLADDR  156 /* Set interface link-level address. */
#define PRIV_NET_ADDIFGROUP 157 /* Add new interface group. */
#define PRIV_NET_DELIFGROUP 158 /* Delete interface group. */
#define PRIV_NET_IFCREATE   159 /* Create cloned interface. */
#define PRIV_NET_IFDESTROY  150 /* Destroy cloned interface. */
#define PRIV_NET_ADDIFADDR  161 /* Add protocol addr to interface. */
#define PRIV_NET_DELIFADDR  162 /* Delete protocol addr on interface. */
#define PRIV_NET_LAGG       163 /* Administer lagg interface. */
#define PRIV_NET_GIF        164 /* Administer gif interface. */
#define PRIV_NET_SETIFVNET  165 /* Move interface to vnet. */
#define PRIV_NET_SETIFDESCR 166 /* Set interface description. */
#define PRIV_NET_SETIFFIB   167 /* Set interface fib. */

/*
 * IPv4 and IPv6 privileges.
 */
#define PRIV_NETINET_RESERVEDPORT 170 /* Bind low port number. */
#define PRIV_NETINET_IPFW   171 /* Administer IPFW firewall. */
#define PRIV_NETINET_DIVERT 172 /* Open IP divert socket. */
#define PRIV_NETINET_PF     173 /* Administer pf firewall. */
#define PRIV_NETINET_DUMMYNET 174 /* Administer DUMMYNET. */
#define PRIV_NETINET_CARP   175 /* Administer CARP. */
#define PRIV_NETINET_MROUTE 175 /* Administer multicast routing. */
#define PRIV_NETINET_RAW    177 /* Open netinet raw socket. */
#define PRIV_NETINET_GETCRED 178 /* Query netinet pcb credentials. */
#define PRIV_NETINET_ADDRCTRL6  179 /* Administer IPv6 address scopes. */
#define PRIV_NETINET_ND6    180 /* Administer IPv6 neighbor disc. */
#define PRIV_NETINET_SCOPE6 181 /* Administer IPv6 address scopes. */
#define PRIV_NETINET_ALIFETIME6 182 /* Administer IPv6 address lifetimes. */
#define PRIV_NETINET_IPSEC  183 /* Administer IPSEC. */
#define PRIV_NETINET_REUSEPORT  184 /* Allow [rapid] port/address reuse. */
#define PRIV_NETINET_SETHDROPTS 185 /* Set certain IPv4/6 header options. */
#define PRIV_NETINET_BINDANY    186 /* Allow bind to any address. */

/*
 * Priv bitmap size.
 */
#define _PRIV_MENT 200
#define _PRIV_MLEN E2BITMAP_SIZE(_PRIV_MENT)

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

enum priv_pcap_mode {
    PRIV_PCAP_MODE_GETR = 0, /* get restr */
    PRIV_PCAP_MODE_SETR = 1, /* set restr */
    PRIV_PCAP_MODE_CLRR = 2, /* clear restr */
    PRIV_PCAP_MODE_GETG = 3, /* get grant */
    PRIV_PCAP_MODE_SETG = 4, /* set grant */
    PRIV_PCAP_MODE_CLRG = 5  /* clear grant */
};

/**
 * Argument struct for SYSCALL_PRIV_PCAP
 */
struct _priv_pcap_args {
    pid_t pid;
    enum priv_pcap_mode mode;
    size_t priv;
};
#endif


#ifdef KERNEL_INTERNAL

#include <sys/param.h>
#include <bitmap.h>

struct cred {
    uid_t uid, euid, suid;
    gid_t gid, egid, sgid;
    gid_t sup_gid[NGROUPS_MAX];
#ifdef configPROCCAP
    bitmap_t pcap_restrmap[_PRIV_MLEN]; /*!< Privilege restrict bitmap. */
    bitmap_t pcap_grantmap[_PRIV_MLEN]; /*!< Privilege grant bitmap. */
#endif
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

/**
 * Initialize a cred struct.
 * @param cred is a pointer to an uninitialized cred struct.
 */
void priv_cred_init(struct cred * cred);

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
int priv_setpcap(pid_t pid, int grant, size_t priv, int value);
int priv_getcap(pid_t pid, int grant, size_t priv);
#endif

#endif /* !_SYS_PRIV_H_ */

/**
 * @}
 */

/**
 * @}
 */
