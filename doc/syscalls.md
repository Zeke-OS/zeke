System Calls
============

Introduction to System Calls
----------------------------

System calls are the main method of communication between the kernel and
user space applications. The word syscall is usually used as a shorthand
for system call in the context of Zeke and in general.

### System call flow

1.  User scope thread makes a syscall by calling:
    `syscall(SYSCALL_XXX_YYY, &args)`, where XXX is generally a
    module/compilation unit name, YYY is a function name and args is a
    syscall dataset structure in format declared in `syscalldef.h`.

2.  The interrupt handler calls `_intSyscall_handler()` function where
    syscall handler of the correct subsystem is resolved from
    `syscall_callmap`.

3.  Execution enters to the subsystem specific `XXX_syscall()` function
    where the system call is either handled directly or a next level
    system call function is called according to minor number of the
    system call.

4.  `XXX_syscall()` returns a `uint32_t` value which is, after multiple
    return steps, returned back to the caller which should know what
    type the returned value actually represents. In the future return
    value should be always integer value having the same size as
    register in the architecture. Everything else shall be passed both
    ways by using args structs, thus unifying the return value.

### Syscall Major and Minor codes

System calls are divided to major and minor codes so that major codes
represents a set of functions related to each other, usually all the
syscall functions in a single compilation unit. Major number sets are
internally called groups. Both numbers are defined in `syscall.h` file.

Adding new syscalls and handlers
--------------------------------

### A new syscall

  - `include/syscall.h` contains syscall number definitions

  - `include/syscalldef.h` contains some useful structures that can be
    used when creating a new syscall

  - Add the new syscall under a syscall group handler

### A new syscall handler

  - Create a new syscall group into `include/syscall.h`

  - Create syscall number definitions into the previous file

  - Add the new syscall group to the list of syscall groups in
    `syscall.c`

  - Create a new syscall group handler

## Libc: System calls

### sysctl

Top level sysctl name spaces:

  - `debug`debugging information.
  - `hw` hardware and device driver information.
  - `kern` Kernel behavior tuning.
  - `machdep` machine-dependent configuration parameters.
  - `net` network subsystem. (Not implemented)
  - `security` security and security-policy configuration and information.
  - `sysctl` reserved name space for the implementation of sysctl.
  - `user` configuration parameters affecting to user applications.
  - `vfs` virtual file system configuration.
  - `vm` virtual memory.
