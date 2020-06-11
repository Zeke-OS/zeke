proc: Process and Thread Management
===================================

Processes and Threads
---------------------

The process and thread management in Zeke is based on an idea that a
process is a container for threads owned by the process and the
process manages the memory space and resources used by its threads.
Naturally this means that a process must always own at least one thread,
that is the main thread and all other threads are children of the main thread.
While a process is an unit or a context of resource allocation, a thread is an
object describing the execution state, in other words a unit of execution, of
an user space application. Any process not having a main thread is immediately
removed from the system because there is nothing to execute.

Since a process is a container containing the main thread, and other
threads, there is no global execution state for a process, like in some
other operating systems. Some operating systems may use the main thread
status as the process state but Zeke makes an isolation between the
process state and the thread state. Thus a currently running process is
mostlikely in `READY` state for most of the time.

### A thread

#### The thread management concept

![Thread tree example](pics/proc-classdiag.svg)

<span label="figure:thtree">**Thread tree example.**</span>

The following notation is used:

  - `tid_X` = Thread ID

  - `pid_a` = Process ID

  - `0` = NULL

`process_a` a has a main thread called `main`. Thread `main` has two
child thread called `child_1` and `child_2`. `child_2` has created one
child thread called `child_2_1`.

`main` owns all the threads it has created and at the same time child
threads may own their own threads. If parent thread is killed then the
children of the parent are killed first in somewhat reverse order.

  - `parent` = Parent thread of the current thread if any

  - `first_child` = First child created by the current thread

  - `next_child` = Next child in chain of children created by the parent
    thread

##### Examples

**process\_a is killed**

Before `process_a` can be killed `main` thread must be killed, because
it has child threads its children has to be resolved and killed in
reverse order of creation.

**child\_2 is killed**

Killing of `child_2` causes `child_2_1` to be killed first.

### A process

![Process states](pics/proc-states.svg)

<span label="figure:procstates">**Process states.**</span>

Figure [\[figure:procstates\]](#figure:procstates) shows every possible
state transition of a process in Zeke.

Calling `fork` syscall causes a creation of a new process container,
cloning of the currently executing thread as a new main thread as well
as marking all current memory regions as copy-on-write regions for both
processes. Marking all currently mapped regions as COW makes it easy to
get rid of otherwise hard to solve race conditions.

When a process calls `exec` syscall the current main thread is replaced
by a newly created main thread that’s pointing to the new process image,
ie. PC is set to the entry point of the new image. Before `exec` transfers
control to the new program the kernel will run resource cleanups and
inherit properties as mandated by the
<span data-acronym-label="POSIX" data-acronym-form="singular+abbrv">POSIX</span>
standard.

### In-kernel User Credential Controls

TODO

Scheduling
----------

### Thread scheduler

The scheduler in Zeke is conceptually object oriented and all scheduling
entities, CPUs as well as scheduling policies are implemented as objects
in the scheduling system. Each CPU can be technically populated with a
different set of scheduling policies that are then only executable on
that particular CPU, see figure
[\[figure:objscheds\]](#figure:objscheds).

![Thread states in the scheduler](pics/thread-states.svg)

<span label="figure:threadstates">**Thread states in the scheduler.**</span>

```
          +----------+
+----------+  CPU1   |
|   CPU0   |+------+ |
| +------+ || FIFO | |
| | FIFO | |+------+ |
| +------+ |+------+ |
| +------+ ||  RR  | |
| |  RR  | |+------+ |
| +------+ |+------+ |
| +------+ || OTTH1| |
| | OTH1 | |+------+ |
| +------+ |---------+
+----------+
```

<span label="figure:objscheds">**Scheduler CPU objects for two processor cores
and per scheduler object scheduling policy objects in priority order.**</span>

Executable File Formats
-----------------------

### Introduction to executables in Zeke

The kernel has a support for implementing loader functions for any new
executable format but currently only 32bit elf support exist. Loading of a new
process image is invoked by calling exec syscall call that calls `load_image()`
function in `kern/exec.c` file.  Process image loaders are advertised by using
`EXEC_LOADER` macro.

### ELF32

The elf loader in Zeke can be used to load statically linked executables as well
as anything linked dynamically. Currently only two loadable sections can be
loaded, code region and heap region.

#### Supported elf sections

The kernel reads process memory regions based on information provided by
`PT_LOAD` sections in the elf file.

The kernel can read additional information needed for executing a binary
from the elf notes. The non-standard notes are only parsed if `Zeke` is
defined as an owner of the note.

**NT_VERSION**

**NT_STACKSIZE**

`NT_STACKIZE` note can be used to hint the kernel about the preferred
minimum size for the main thread stack.

**NT_CAPABILITIES**

`NT_CAPABILITIES` note is used to inform the kernel about capabilities
required to execute a binary file. The elf loader attempts to set each
capability as an effective capability, which may fail if the capability
isn’t in the bounding capabilities set. In case the file has
`O_EXEC_ALTPCAP` `oflag` set then the loader will first
add the capabilities found in these notes to the bounding capabilities
set, i.e. the executable can gain the bounding capabilities.

**NT_CAPABILITIES_REQ**

`NT_CAPABILITIES_REQ` note functions similar to `NT_CAPABILITIES` but
it doesn’t allow new bounding capabilities to be gained even when the
binary file is opened with `O_EXEC_ALTPCAP`.

The user space implementation of these note types is discussed in
the Libc: ELF support section.

Libc: Process and Thread Management
-----------------------------------

### ELF support

Zeke libc supports adding simple note sections to elf
files by using macros provided by `sys/elf_notes.h` header file. As
discussed previously in chapter [](#chap:exec), some additional
information about the runtime environment requirements can be passed via
note sections. For example, a macro called `ELFNOTE_STACKSIZE` can be
used for indicating the minimum stack size needed by the main tread of
an executable, see listing [\[list:hugestack\]](#list:hugestack).

Another important Zeke specific note type is `NT_CAPABILITIES` that can
be used to annotate the capabilities required to successfully execute a
given elf binary file. The capability notes can be created using the
`ELFNOTE_CAPABILITIES(...)` macro. Each note can have 64 capabilities
and the total number of these notes is unlimited. Depending on the file
system if the file is marked as a system file the binary can gain these
capabilities as bounding and effective capabilities, similar to `suid`
allowing a program to gain privileges of another user.

The `NT_CAPABILITIES_REQ` note type is similar to `NT_CAPABILITIES` but
it doesn’t allow gaining new bounding capabilities. This note can be
created using `ELFNOTE_CAPABILITIES_REQ(...)` macro.

**hugestack.c**

```c
#include <stdio.h>
#include <sys/elf_notes.h>

ELFNOTE_STACKSIZE(16384);

void recurse(int level)
{
    char str[1024];

    sprintf(str, "r %d", level);
    puts(str);

    if (level < 10)
        recurse(level + 1);
}

int main(void)
{
    recurse(1);

    return 0;
}
```

### Fork and exec

#### Creating a daemon

Creating a daemon is probably one of the main features of operating
systems like Zeke. The preferred way of creating a daemon in Zeke is
forking once and creating a new session for the child process by calling
`setsid()`. The parent process may exit immediately after forking.
Listing [\[list:daemon\]](#list:daemon) shows an example of a daemon
creation procedure.

**daemon.c**

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char * argv[])
{
    pid_t pid, sid;
    FILE * fp;

    pid = fork();
    if (pid < 0) {
        printf("fork failed!\n");
        exit(1);
    }
    if (pid > 0) {
        printf("pid of child process %d\n", pid);
        exit(0);
    }

    umask(0);
    sid = setsid();
    if (sid < 0) {
        exit(1);
    }

    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    fp = fopen("/tmp/daemon_log.txt", "w+");

    while (1) {
        fprintf(fp, "Logging info...\n");
        fflush(fp);
        sleep(10);
    }

    fclose(fp);

    return 0;
}
```

### User credentials control

TODO

### Pthread

TODO

#### Thread creation and destruction

TODO

#### Thread local storage

TODO

#### Mutex

> Tis in my memory lock'd,<br/>
> And you yourself shall keep the key of it.

*Hamlet*, 1, ii
