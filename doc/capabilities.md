priv: Capabilities
==================

By default there is no notion of a super user in Zeke in a traditional way.
Instead of being a magic super user, the `root` user is handled like a
regular user by the kernel. Therefore, there is no magic in having `uid=0`.
However, since we still do have a `root` user in the system, it's partially
emulated by user space in the `login` command.

Before going any further on what this all means, let's look into how Zeke's
capabilites work.

[sys/priv.h](/include/sys/priv.h) defines a number of capabilities that gives
a process (or a user) some elevated privileges to commit system maintenance
tasks and inspect other users of the system. Notably the header file also
declares the following functions to inspect and manipulate the capabilities
of the current process:

- `int priv_setpcap(int bounding, size_t priv, int value)`
- `int priv_getpcap(int bounding, size_t priv)`
- `int priv_rstpcap(void)`
- `int priv_getpcaps(uint32_t * effective, uint32_t * bounding)`

A process has a set of effective capabilities and a set of bounding
capabilities. During normal operation the process can only act using the
capabilities that are effective. When execing a new process the process
may inherit the previous effective set of capabilities and gain new
capabilities from the bounding set.

The first way to gain new bounding capabilities is by calling
`priv_setpcap(true, priv, value)` while already holding the `PRIV_SETBND`
capability that allows settings new bounding capabilities. Otherwise the
call will fail.

The other way to gain new bounding capabilities is when an elf file is
executed. This requires a few things. First the elf file must have an
`NT_CAPABILITIES` section declaring the required capabilities. Second the
elf file (handle) must have `O_EXEC_ALTPCAP` flag set. This flag can be
only set by the kernel, as it's outside of `O_USERMASK`. How the flag
is set is file system dependant, on `fatfs` it's set when the `system`
attribute of the file is set.

So how the `login` command works is that it has `system` attribute set
in the rootfs image and it can therefore request new bounding and effective
capabilities when it's executed. Then when a user logs in unncessary
capabilities will be dropped.

Such an elf note for requesting or requiring capabilities can be created
as follows:

```c
#include <sys/elf_notes.h>
#include <sys/priv.h>

ELFNOTE_CAPABILITIES(
    PRIV_CLRCAP,
    PRIV_SETBND,
    PRIV_EXEC_B2E,
    PRIV_CRED_SETUID,
    PRIV_CRED_SETEUID,
    PRIV_CRED_SETSUID,
    PRIV_CRED_SETGID,
    PRIV_CRED_SETEGID,
    PRIV_CRED_SETSGID,
    PRIV_CRED_SETGROUPS,
    PRIV_PROC_SETLOGIN,
    PRIV_SIGNAL_ACTION,
    PRIV_TTY_SETA,
    PRIV_VFS_READ,
    PRIV_VFS_WRITE,
    PRIV_VFS_EXEC,
    PRIV_VFS_LOOKUP,
    PRIV_VFS_STAT,
    PRIV_SIGNAL_OTHER,
    PRIV_SIGNAL_ACTION,
);
```

See the [Libc: ELF support section](proc.md) in the proc documentation for more
information about the libc userspace interface.
