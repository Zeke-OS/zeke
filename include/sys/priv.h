/**
 *******************************************************************************
 * @file    priv.h
 * @author  Olli Vanhoja
 * @brief   User credentials.
 * @section LICENSE
 * Copyright (c) 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/*
 * Privilege checking interface for BSD kernel.
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
#define PRIV_ACCT            1 /* Manage process accounting. */
#define PRIV_MAXFILES        2 /* Exceed system open files limit. */
#define PRIV_MAXPROC         3 /* Exceed system processes limit. */
#define PRIV_KTRACE          4 /* Set/clear KTRFAC_ROOT on ktrace. */
#define PRIV_SETDUMPER       5 /* Configure dump device. */
#define PRIV_REBOOT          6 /* Can reboot system. */
#define PRIV_SWAPON          7 /* Can swapon(). */
#define PRIV_SWAPOFF         8 /* Can swapoff(). */
#define PRIV_MSGBUF          9 /* Can read kernel message buffer. */
#define PRIV_IO             10 /* Can perform low-level I/O. */
#define PRIV_KEYBOARD       11 /* Reprogram keyboard. */
#define PRIV_DRIVER         12 /* Low-level driver privilege. */
#define PRIV_ADJTIME        13 /* Set time adjustment. */
#define PRIV_NTP_ADJTIME    14 /* Set NTP time adjustment. */
#define PRIV_CLOCK_SETTIME  15 /* Can call clock_settime. */
#define PRIV_SETTIMEOFDAY   16 /* Can call settimeofday. */

/*
 * Credential management privileges.
 */
#define PRIV_CRED_SETUID    20 /* setuid. */
#define PRIV_CRED_SETEUID   21 /* seteuid to !ruid and !svuid. */
#define PRIV_CRED_SETGID    22 /* setgid. */
#define PRIV_CRED_SETEGID   23 /* setgid to !rgid and !svgid. */
#define PRIV_CRED_SETGROUPS 24 /* Set process additional groups. */
#define PRIV_CRED_SETREUID  25 /* setreuid. */
#define PRIV_CRED_SETREGID  26 /* setregid. */
#define PRIV_CRED_SETRESUID 27 /* setresuid. */
#define PRIV_CRED_SETRESGID 28 /* setresgid. */

/*
 * Loadable kernel module privileges.
 */
#define PRIV_KLD_LOAD       40 /* Load a kernel module. */
#define PRIV_KLD_UNLOAD     41 /* Unload a kernel module. */

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
 * System V IPC privileges.
 * POSIX message queue privileges.
 * POSIX semaphore privileges.
 */
#define PRIV_IPC_READ       70 /* Can override IPC read perm. */
#define PRIV_IPC_WRITE      71 /* Can override IPC write perm. */
#define PRIV_IPC_ADMIN      72 /* Can override IPC owner-only perm. */
#define PRIV_IPC_MSGSIZE    73 /* Exempt IPC message queue limit. */
#define PRIV_MQ_ADMIN       74 /* Can override msgq owner-only perm. */
#define PRIV_SEM_WRITE      75 /* Can override sem write perm. */

/*
 * Performance monitoring counter privileges.
 */
#define PRIV_PMC_MANAGE     80 /* Can administer PMC. */
#define PRIV_PMC_SYSTEM     81 /* Can allocate a system-wide PMC. */

/*
 * Scheduling privileges.
 */
#define PRIV_SCHED_DIFFCRED 90  /* Exempt scheduling other users. */
#define PRIV_SCHED_SETPRIORITY 91 /* Can set lower nice value for proc. */
#define PRIV_SCHED_RTPRIO   92  /* Can set real time scheduling. */
#define PRIV_SCHED_SETPOLICY 93 /* Can set scheduler policy. */
#define PRIV_SCHED_SET      94  /* Can set thread scheduler. */
#define PRIV_SCHED_SETPARAM 95  /* Can set thread scheduler params. */

/*
 * Signal privileges.
 */
#define PRIV_SIGNAL_DIFFCRED 100 /* Exempt signalling other users. */
#define PRIV_SIGNAL_SUGID   101 /* Non-conserv signal setuid proc. */

/*
 * Sysctl privileges.
 */
#define PRIV_SYSCTL_DEBUG   110 /* Can invoke sysctl.debug. */
#define PRIV_SYSCTL_WRITE   111 /* Can write sysctls. */

/*
 * TTY privileges.
 */
#define PRIV_TTY_CONSOLE    120 /* Set console to tty. */
#define PRIV_TTY_DRAINWAIT  121 /* Set tty drain wait time. */
#define PRIV_TTY_DTRWAIT    122 /* Set DTR wait on tty. */
#define PRIV_TTY_EXCLUSIVE  123 /* Override tty exclusive flag. */
#define PRIV_TTY_STI        125 /* Simulate input on another tty. */
#define PRIV_TTY_SETA       126 /* Set tty termios structure. */

/*
 * VFS privileges.
 */
#define PRIV_VFS_READ       130 /* vnode read perm. */
#define PRIV_VFS_WRITE      131 /* vnode write perm. */
#define PRIV_VFS_ADMIN      132 /* vnode admin perm. */
#define PRIV_VFS_EXEC       133 /* vnode exec perm. */
#define PRIV_VFS_LOOKUP     134 /* vnode lookup perm. */
#define PRIV_VFS_BLOCKRESERVE 135 /* Can use free block reserve. */
#define PRIV_VFS_CHFLAGS_DEV  136 /* Can chflags() a device node. */
#define PRIV_VFS_CHOWN      137 /* Can set user; group to non-member. */
#define PRIV_VFS_CHROOT     138 /* chroot(). */
#define PRIV_VFS_RETAINSUGID 139 /* Can retain sugid bits on change. */
#define PRIV_VFS_EXTATTR_SYSTEM 140 /* Operate on system EA namespace. */
#define PRIV_VFS_LINK       141 /* bsd.hardlink_check_uid */
#define PRIV_VFS_MKNOD_BAD  142 /* Can mknod() to mark bad inodes. */
#define PRIV_VFS_MKNOD_DEV  143 /* Can mknod() to create dev nodes. */
#define PRIV_VFS_MKNOD_WHT  144 /* Can mknod() to create whiteout. */
#define PRIV_VFS_MOUNT      145 /* Can mount(). */
#define PRIV_VFS_MOUNT_OWNER    146 /* Can manage other users' file systems. */
#define PRIV_VFS_MOUNT_EXPORTED 147 /* Can set MNT_EXPORTED on mount. */
#define PRIV_VFS_MOUNT_PERM     148 /* Override dev node perms at mount. */
#define PRIV_VFS_MOUNT_SUIDDIR  149 /* Can set MNT_SUIDDIR on mount. */
#define PRIV_VFS_MOUNT_NONUSER  150 /* Can perform a non-user mount. */
#define PRIV_VFS_SETGID     151 /* Can setgid if not in group. */
#define PRIV_VFS_STICKYFILE 152 /* Can set sticky bit on file. */
#define PRIV_VFS_SYSFLAGS   153 /* Can modify system flags. */
#define PRIV_VFS_UNMOUNT    154 /* Can unmount(). */
#define PRIV_VFS_STAT       155 /* Stat perm. */

/*
 * Virtual memory privileges.
 */
#define PRIV_VM_MADV_PROTECT 160 /* Can set MADV_PROTECT. */
#define PRIV_VM_MLOCK        161 /* Can mlock(), mlockall(). */
#define PRIV_VM_MUNLOCK      162 /* Can munlock(), munlockall(). */
#define PRIV_VM_SWAP_NOQUOTA 163 /*
                                  * Can override the global
                                  * swap reservation limits.
                                  */
#define PRIV_VM_SWAP_NORLIMIT 164 /*
                                   * Can override the per-uid swap reservation
                                   * limits.
                                   */

/*
 * Device file system privileges.
 */
#define PRIV_DEVFS_RULE     170 /* Can manage devfs rules. */
#define PRIV_DEVFS_SYMLINK  171 /* Can create symlinks in devfs. */

/*
 * Random number generator privileges.
 */
#define PRIV_RANDOM_RESEED  180 /* Closing /dev/random reseeds. */

/*
 * Network stack privileges.
 */
#define PRIV_NET_BRIDGE     190 /* Administer bridge. */
#define PRIV_NET_GRE        191 /* Administer GRE. */
#define _PRIV_NET_PPP       192 /* Removed. */
#define _PRIV_NET_SLIP      193 /* Removed. */
#define PRIV_NET_BPF        194 /* Monitor BPF. */
#define PRIV_NET_RAW        195 /* Open raw socket. */
#define PRIV_NET_ROUTE      196 /* Administer routing. */
#define PRIV_NET_TAP        197 /* Can open tap device. */
#define PRIV_NET_SETIFMTU   198 /* Set interface MTU. */
#define PRIV_NET_SETIFFLAGS 199 /* Set interface flags. */
#define PRIV_NET_SETIFCAP   200 /* Set interface capabilities. */
#define PRIV_NET_SETIFNAME  201 /* Set interface name. */
#define PRIV_NET_SETIFMETRIC 202 /* Set interface metrics. */
#define PRIV_NET_SETIFPHYS  203 /* Set interface physical layer prop. */
#define PRIV_NET_SETIFMAC   204 /* Set interface MAC label. */
#define PRIV_NET_ADDMULTI   205 /* Add multicast addr. to ifnet. */
#define PRIV_NET_DELMULTI   206 /* Delete multicast addr. from ifnet. */
#define PRIV_NET_HWIOCTL    207 /* Issue hardware ioctl on ifnet. */
#define PRIV_NET_SETLLADDR  208 /* Set interface link-level address. */
#define PRIV_NET_ADDIFGROUP 209 /* Add new interface group. */
#define PRIV_NET_DELIFGROUP 210 /* Delete interface group. */
#define PRIV_NET_IFCREATE   211 /* Create cloned interface. */
#define PRIV_NET_IFDESTROY  212 /* Destroy cloned interface. */
#define PRIV_NET_ADDIFADDR  213 /* Add protocol addr to interface. */
#define PRIV_NET_DELIFADDR  214 /* Delete protocol addr on interface. */
#define PRIV_NET_LAGG       215 /* Administer lagg interface. */
#define PRIV_NET_GIF        216 /* Administer gif interface. */
#define PRIV_NET_SETIFVNET  217 /* Move interface to vnet. */
#define PRIV_NET_SETIFDESCR 218 /* Set interface description. */
#define PRIV_NET_SETIFFIB   219 /* Set interface fib. */

/*
 * IPv4 and IPv6 privileges.
 */
#define PRIV_NETINET_RESERVEDPORT 220 /* Bind low port number. */
#define PRIV_NETINET_IPFW   221 /* Administer IPFW firewall. */
#define PRIV_NETINET_DIVERT 222 /* Open IP divert socket. */
#define PRIV_NETINET_PF     223 /* Administer pf firewall. */
#define PRIV_NETINET_DUMMYNET 224 /* Administer DUMMYNET. */
#define PRIV_NETINET_CARP   225 /* Administer CARP. */
#define PRIV_NETINET_MROUTE 225 /* Administer multicast routing. */
#define PRIV_NETINET_RAW    227 /* Open netinet raw socket. */
#define PRIV_NETINET_GETCRED 228 /* Query netinet pcb credentials. */
#define PRIV_NETINET_ADDRCTRL6 229 /* Administer IPv6 address scopes. */
#define PRIV_NETINET_ND6    230 /* Administer IPv6 neighbor disc. */
#define PRIV_NETINET_SCOPE6 231 /* Administer IPv6 address scopes. */
#define PRIV_NETINET_ALIFETIME6 232 /* Administer IPv6 address lifetimes. */
#define PRIV_NETINET_IPSEC  233 /* Administer IPSEC. */
#define PRIV_NETINET_REUSEPORT  234 /* Allow [rapid] port/address reuse. */
#define PRIV_NETINET_SETHDROPTS 235 /* Set certain IPv4/6 header options. */
#define PRIV_NETINET_BINDANY    236 /* Allow bind to any address. */

/*
 * cpuctl(4) privileges.
 */
#define PRIV_CPUCTL_WRMSR   240 /* Write model-specific register. */
#define PRIV_CPUCTL_UPDATE  241 /* Update cpu microcode. */

/*
 * mem(4) privileges.
 */
#define PRIV_KMEM_READ      250 /* Open mem/kmem for reading. */
#define PRIV_KMEM_WRITE     251 /* Open mem/kmem for writing. */

/*
 * Priv bitmap size.
 */
#define _PRIV_MSIZE     256

#ifdef KERNEL_INTERNAL
#ifndef FS_H
#error fs.h must be included
#endif

struct proc_info;

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

/**
 * Check privileges.
 * @param proc is a pointer to a process.
 * @param priv is a PRIV.
 * @return Typically, 0 will be returned for success, and -EPERM will be
 *         returned on failure. In case of invalid arguments -EINVAL is
 *         returned.
 */
int priv_check(struct proc_info * proc, int priv);

/**
 * @}
 */

#else
int priv_setpcap(pid_t pid, int grant, size_t priv, int value);
int priv_getcap(pid_t pid, int grant, size_t priv);
#endif

#endif /* !_SYS_PRIV_H_ */
