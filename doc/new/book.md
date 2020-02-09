Introduction to Zeke
====================

Zeke is a portable operating system mostly targetted for ARM
processors.\[1\] Zero Kernel, as it was originally known, is a tiny
kernel implementation that was originally targeted for ARM Corex-M
microcontrollers. The reason to start this project was that most of
RTOSes available at the time for M0 feeled less or more bloated for very
simple applications. Secondly I found architectures based on ARMv6-M to
be quite challenging and interesting platforms for RTOS/kernel
development, especially when ARMv6-M is compared to ARMv7-M used in M4
core or any Cortex-A cores using real ARM architectures.

One of the original goals of Zero Kernel was to make it CMSIS-RTOS
compliant where possible, as some concepts of Zeke were not actually
CMSIS compliant from the begining. However the scope of the project
shifted pretty early and the kernel is no longer CMSIS compatible at any
level. Currently Zeke is moving towards POSIX-like system and its user
space is taking a Unix-like shape. Nowadays Zeke is a bit bloatty when
compared to the original standard of a bloated OS but I claim Zeke is
still quite tightly integrated system, compared to any other Unix-like
OS implementation.

Figure [\[figure:zeke\]](#figure:zeke) illustrates architectural layers
of the operating system. Scope of Zeke project is to implement a
portable Unix-like\[2\] operating system from scratch\[3\] that is
optimized for ARM architectures and is free of legacy.

In addition to portability another long standing goal has been
configurability and adjustable footprint of the kernel binary as well as
the whole OS image. This is achieved by modular kernel architecture and
static compile time configuration. Like for example
<span data-acronym-label="HAL" data-acronym-form="singular+short">HAL</span>
selection is already almost fully locked at compilation time.

Zeke includes a partially BSD compatible C standard library.

![Zeke: a Portable Operating
System.<span label="figure:zeke"></span>](pics/zeke)

### List of Abbreviations and Meanings

- \[POSIX\] <span>Portable Operating System Interface for uniX</span> is a family of
  standards specifying a standard set of features for compatibility
  between operating systems.
- \[HAL\] <span>Hardware Abstraction Layer</span>.

- \[inode\] <span>inode</span> is the file index node in Unix-style file systems that is
- used to represent a file fie system object.
- \[LFN\] <span>Long File Name</span>.
- \[MBR\] <span>Master Boot Record</span>.
- \[ramfs\] <span>RAM file system</span>.
- \[SFN\] <span>Short File Name</span>.
- \[VFS\] <span>Virtual File System</span>.
- \[vnode\] <span>vnode</span> is like an inode but it's used as an abstraction
  level for compatibility between different file systems in Zeke. Particularly
  it's used at VFS level.

- \[bio\] <span>IO Buffer Cache</span>.
- \[dynmem\] (<span>dynamic memory</span>) is the block memory allocator system in Zeke,
  allocating in 1 MB blocks.
- \[VRAlloc\] or \[vralloc] <span>Virtual Region Allocator</span>.

- \[MIB\] <span>Management Information Base</span> tree.

- \[IPC\] </span>Inter-Process Communication</span>.
- \[shmem\] <span>Shared Memory</span>.
- \[RCU\] <span>Read-Copy-Update</span>.

- \[ELF\] <span>Executable and Linkable or Extensible Linking Format</span>.

- \[PUnit\] a portable unit testing framework for C.
- \[GCC\] GNU Compiler Collection.
- \[GLIBC\] The GNU C Library.
- \[CPU\] <span>Central Processing Unit</span> is, generally speaking, the main
  processor of some computer or embedded system.

- \[DMA\] <span>Direct Memory Access</span> is a feature that allows hardware
  subsystems to commit memory transactions without CPU's intervention.
- \[PC\] <span>Program Counter</span> register.

Bootstrapping
=============

Bootloader
----------

TODO

Kernel Initialization
---------------------

### Introduction

Kernel initialization order is defined as follows:

1. `hw_preinit`
2. constructors
3. `hw_postinit`
4. `kinit()`
5. uinit thread

After kinit, scheduler will kick in and initialization continues in
`sinit(8)` process that is bootstrapped by uinit thread.

### Kernel module/subsystem initializers

There are currently four types of initializers supported:

- **hw\_preinit** for mainly hardware related initializers
- **hw\_postinit** for hardware related initializers
- **constructor** (or init) for generic initialization

Every initializer function should contain `SUBSYS_INIT("XXXX init");`
before any functional code and optionally before that a single or
multiple `SUBSYS_DEP(YYYY_init);` lines declaring subsystem
initialization dependencies.

Descturctors are not currently supported in Zeke but if there ever will
be LKM support the destructors will be called when unloading the module.

Listing [\[list:kmodinit\]](#list:kmodinit) shows the
constructor/intializer notation used by Zeke subsystems. The initializer
function can return a negative errno code to indicate an error, but
`-EAGAIN` is reserved to indicate that the initializer was already
executed.

```c
int __kinit__ mod_init(void)
{
    SUBSYS_DEP(modx_init); /* Module dependency */
    SUBSYS_INIT("module name");
    ...
    return 0;
}

void mod_deinit(void) __attribute__((destructor))
{
    ...
}
```

Constructor prioritizing is not supported and `SUBSYS_DEP` should be
used instead to indicate initialization dependecies.

hw\_preinit and hw\_postinit can be used by including `kinit.h` header
file and using the notation as shown in
[\[list:hwprepostinit\]](#list:hwprepostinit). These should be rarely
needed and used since preinit doesn’t allow many kernel features to be
used and postinit can be called after the scheduler is already running.

```c
int mod_preinit(void)
{
    SUBSYS_INIT("hw driver");
    ...
    return 0;
}
HW_PREINIT_ENTRY(mod_preinit);

int mod_postinit(void)
{
    SUBSYS_INIT("hw driver");
    ...
    return 0;
}
HW_POSTINIT_ENTRY(mod_postinit);
```

Userland Initialization
-----------------------

### Introduction

TODO

- `init`
- `sbin/rc.init`
- `getty` and `gettytab`
- `login`

Memory Management
=================

An Overview to Memory Management in Zeke
----------------------------------------

In this chapter we will briefly introduce the architectural layout of
memory management in Zeke. Zeke as well as most of major operating
systems divides its memory management and mapping to several layers to
hide away any hardware differences and obscurities. In Zeke these layers
are
<span data-acronym-label="MMU" data-acronym-form="singular+short">MMU</span>
<span data-acronym-label="HAL" data-acronym-form="singular+abbrv">HAL</span>
that abstracts the hardware, `dynmem` handling the dynamic allocation of
contiguous blocks of memory and `kmalloc` that allocates memory for the
kernel itself, and probably most importantly `vralloc/buf/bio` subsystem
that’s handling all allocations for processes and IO buffers. Relations
between different kernel subsystems using and implementing memory
management are shown in figure [\[figure:vmsubsys\]](#figure:vmsubsys).

```
    U                      +---------------+
    S                      |    malloc     |
    R                      +---------------+
    ------------------------------ | -----------
    K   +---------------+  +---------------+
    E   |    kmalloc    |  |     proc      |
    R   +---------------+  +---------------+
    N           |     /\    |      |
    E           |     |    \/      |
    L  +-----+  |  +---------+   +----+
       | bio |--|--| vralloc |---| vm |
       +-----+  |  +---------+   +----+
                |     |            |
               \/    \/            \/
        +---------------+     +----------+
        |    dynmem     |-----| ptmapper |
        +---------------+     +----------+
                |                  |
               \/                  |
        +---------------+          |
        |    mmu HAL    |<----------
        +---------------+
                |
        +-----------------------+
        | CPU specific MMU code |
        +-----------------------+
    ----------- | ------------------------------
    H   +-------------------+
    W   | MMU & coProcessor |
        +-------------------+
```

  - `kmalloc` - is a kernel level memory allocation service, used solely
    for memory allocations in kernel space.

  - `vralloc` - VRAlloc is a memory allocator targeted to allocate
    blocks of memory that will be mapped in virtual address space of a
    processes, but it’s widely used as a generic allocator for medium
    size allocations, it returns a `buf` structs that are used to
    describe the allocation and its state.

  - `bio` - is a IO buffer system, mostly compatible with the
    corresponding interface in BSD kernels, utilizing vralloc and buf
    system.

  - `dynmem` - is a dynamic memory allocation system that allocates &
    frees contiguous blocks of physical memory (1 MB).

  - `ptmapper` - owns all statically allocated page tables (particularly
    the master page table) and regions, and it is also used to allocate
    new page tables from the page table region.

  - `vm` - vm runs various checks on virtual memory access, copies data
    between user land, kernel space and allocates and maps memory for
    processes, and wraps memory mapping operations for proc and
    <span data-acronym-label="bio" data-acronym-form="singular+abbrv">bio</span>.

  - mmu HAL - is an interface to access MMU, provided by `mmu.h` and
    `mmu.c`.

  - CPU specific MMU code is the module responsible of configuring the
    physical MMU layer and implementing the HW interface provided by
    `mmu.h`

<span id="table:bcm_memmap" label="table:bcm_memmap">\[table:bcm\_memmap\]</span>

| Address               |   | Description                |
| :-------------------- | :-: | :------------------------- |
| **Interrupt Vectors** |   |                            |
| 0x0 - 0xff            |   | Not used by Zeke           |
| 0x100 - 0x4000        | L | Typical placement of ATAGs |
| **Priv Stacks**       |   |                            |
| 0x1000 - 0x2fff       | Z | Supervisor (SWI/SVC) stack |
| 0x3000 - 0x4fff       | Z | Abort stack                |
| 0x5000 - 0x5fff       | Z | IRQ stack                  |
| 0x6000 - 0x6fff       | Z | Undef stack                |
| 0x7000 - 0x7fff       | Z | System stack               |
| 0x8000 - 0x3fffff     | Z | Kernel area (boot address) |
| 0x00400000-           | Z | Page Table                 |
| 0x007FFFFF            |   | Area                       |
| 0x00800000            | Z | Dynmem                     |
| 0x00FFFFFF            |   | Area                       |
| \-                    |   |                            |
| **Peripherals**       |   |                            |
| 0x20000000 -          |   |                            |
| *Interrupts*          |   |                            |
| 0x2000b200            | B | IRQ basic pending          |
| 0x2000b204            | B | IRQ pending 1              |
| 0x2000b20c            | B | IRQ pending 2              |
| 0x2000b210            | B | Enable IRQs 1              |
| 0x2000b214            | B | Enable IRQs 2              |
| 0x2000b218            | B | Enable Basic IRQs          |
| 0x2000b21c            | B | Disable IRQs 1             |
| 0x2000b220            | B | Disable IRQs 2             |
| 0x2000b224            | B | Disable Basic IRQs         |
| \- 0x20FFFFFF         | B | Peripherals                |

The memory map of Zeke running on BCM2835.

|   |                                    |
| :- | :--------------------------------- |
|   |                                    |
|   |                                    |
| Z | Zeke specific                      |
| L | Linux bootloader specific          |
| B | BCM2835 firmware specific mappings |

The memory map of Zeke running on BCM2835.

\[MMU\]<span>Memory Management Unit</span>

<span data-acronym-label="HAL" data-acronym-form="singular+abbrv">HAL</span>
----------------------------------------------------------------------------

**ARM11 note:** Only 4 kB pages are used with L2 page tables thus XN
(Execute-Never) bit is always usable also for L2 pages.

#### Domains

See `MMU_DOM_xxx` definitions.

dynmem
------

Dynmem is a memory block allocator that always allocates memory in
\(1 \:\textrm{MB}\) blocks. See fig.
[\[figure:dynmem\_blocks\]](#figure:dynmem_blocks). Dynmem allocates its
blocks as \(1 \:\textrm{MB}\) sections from the L1 kernel master page
table and always returns physically contiguous memory regions. If dynmem
is passed for a thread it can be mapped either as a section entry or via
L2 page table, though this is normally not done as the vm interface is
used instead, though this is normally not done as the vm interface is
used instead.

\definecolor{lightgreen}{rgb}{0.64,1,0.71}

\definecolor{lightred}{rgb}{1,0.7,0.71}

<span>4</span>  
& &
&

\centering

<span id="figure:dynmem_blocks" label="figure:dynmem_blocks">\[figure:dynmem\_blocks\]</span>

kmalloc
-------

The current implementation of a generic kernel memory allocator is
largely based on a tutorial written by Marwan Burrelle. Figure
[\[figure:mm\_layers\]](#figure:mm_layers) shows kmalloc’s relation to
the the memory management stack of the
kernel.

- [Marwan Burelle - A malloc tutorial](http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf)

\centering

<span id="figure:mm_layers" label="figure:mm_layers">\[figure:mm\_layers\]</span>

### The implementation

The current kernel memory allocator implementation is somewhat naive and
exploits some very simple techniques like the first fit algorithm for
allocating memory.

The idea of the first fit algorithm is to find a first large enough free
block of memory from an already allocated region of memory. This is done
by traversing the list of memory blocks and looking for a sufficiently
large block. This is of course quite sub-optimal and better solutions
has to be considered in the future. When a large enough block is found
it’s split in two halves so that the left one corresponds to requested
size and the right block is left free. All data blocks are aligned to 4
byte access.

Fragmentation of memory blocks is kept minimal by immediately merging
newly freed block with neighboring blocks. This approach will keep all
free blocks between reserved blocks contiguous but it doesn’t work if
there is lot of allocations of different sizes that are freed
independently. Therefore the current implementation will definitely
suffer some fragmentation over time.

When kmalloc is out of (large enough) memory blocks it will expand its
memory space by allocating a new block of memory from dynmem. Allocation
is commited in 1 MB blocks (naturally) and always rounded to the next 1
MB.

Kmalloc stores its linked list of reserved and free blocks in the same
memory that is used to allocate memory for its clients. Listing
[\[list:mblockt\]](#list:mblockt) shows the `mblock_t` structure
definition used internally in kmalloc for linking blocks of memory.

```c
typedef struct mblock {
    size_t size;            /* Size of data area of this block. */
    struct mblock * next;   /* Pointer to the next memory block desc. */
    struct mblock * prev;   /* Pointer to the previous memory block desc. */
    int refcount;           /*!< Ref count. */
    void * ptr;             /*!< Memory block descriptor validation.
                             *   This should point to the data. */
    char data[1];
} mblock_t;
```

<span>16</span>  
  
  
  
  
  

<span id="figure:kmalloc_blocks" label="figure:kmalloc_blocks">\[figure:kmalloc\_blocks\]</span>

Descriptor structs are used to store the size of the data block,
reference counters, and pointers to neighbouring block descriptors.

### Suggestions for further development

#### Memory allocation algorithms

The current implementation of kmalloc relies on first-fit algorithm and
variable sized blocks, that are processed as a linked list, which is
obviously inefficient.

One achievable improvement could be adding a second data structure that
would maintain information about free memory blocks that could be used
to store the most common object sizes. This data structure could be also
used to implement something like best-fit instead of first-fit and
possibly with even smaller time complexity than the current
implementation.

\[\begin{aligned}
\mathrm{proposed\_size} &=& \mathrm{req\_size}
  + \frac{\mathrm{curr\_size}}{\mathrm{req\_size}} \mathrm{o\_fact}
  + \frac{\mathrm{curr\_size}}{o\_div}.\end{aligned}\]

<span id="algo:realloc_oc" label="algo:realloc_oc">\[algo:realloc\_oc\]</span>

\If{$\mathrm{req\_size} > \mathrm{proposed\_size}$}

\(\mathrm{new\_size} \gets \mathrm{req\_size}\)
\(\mathrm{new\_size} \gets \mathrm{proposed\_size}\)
\(\mathrm{new\_size} \gets \mathrm{max(req\_size, curr\_size})\)

Figure [\[figure:realloc\]](#figure:realloc) shows "simulations" for a
over committing realloc function. This is however completely untested
and intuitively derived method but it seems to perform sufficiently well
for hypothetical memory allocations.

\center

![New realloc
method.<span label="figure:realloc"></span>](pics/realloc)

vralloc
-------

<span data-acronym-label="vralloc" data-acronym-form="singular+full">vralloc</span>
is a memory allocator used to allocate blocks of memory that can be
mapped in virtual address space of a user processes or used for file
system buffering. Vralloc implements the `geteblk()` part of the
interface described by `buf.h`, meaning allocating memory for generic
use.

`vreg` struct is the intrenal representation of a generic page aligned
allocation made from dynmem and `struct buf` is the external interface
used to pass allocated memory for external users.

``` 
                      last_vreg
                               \
+----------------+     +-----------------+     +-------+
| .paddr = 0x100 |     | .paddr = 0x1500 |     |       |
| .count = 100   |<--->| .count = 20     |<--->|       |
| .map[]         |     | .map[]          |     |       |
+----------------+     +-----------------+     +-------+
```

<span id="figure:vralloc_blocks" label="figure:vralloc_blocks">\[figure:vralloc\_blocks\]</span>

\center

![vralloc and buffer
interface.<span label="figure:vrregbufapi"></span>](gfx/vralloc-buffer)

Virtual Memory
--------------

Each process owns their own master page table, in contrast to some
kernels where there might be only one master page table or one partial
master page table, and varying number of level two page tables. The
kernel, also known as proc 0, has its own master page table that is used
when a process executes in kernel mode, as well as when ever a kernel
thread is executing. Static or fixed page table entries are copied to
all master page tables created. A process shares its master page table
with its childs on `fork()` while `exec()` will trigger a creation of a
new master page table.

Virtual memory is managed as virtual memory buffers (`struct buf`) that
are suitable for in-kernel buffers, IO buffers as well as user space
memory mappings. Additionlly the buffer system supports copy-on-write as
well as allocator schemes where a part of the memory is stored on a
secondary storage (i.e. paging).

Due to the fact that `buf` structures are used in different allocators
there is no global knowledge of the actual state of a particular
allocation, instead each allocator should/may keep track of allocation
structs if desired so. Ideally the same struct can be reused when moving
data from a secondary storage allocator to vralloc memory (physical
memory). However we always know whether a buffer is currently in core or
not (`b_data`) and we also know if a buffer can be swapped to a
different allocator (`B_BUSY` flag).

### Page Fault handling and VM Region virtualization

1.  DAB exception transfers execution to `interrupt_dabt` in
    `XXX_int_handlers.S`

2.  `mmu_data_abort_handler()` (`XXX_mmu.c`) gets called

3.  to be completed...

Libc: Memory Management
-----------------------

Zeke libc provides memory management features comparable to most modern
Unices available. The syscalls that can be used for memory allocation are
`brk()`/`sbrk()` and `mmap()`/`munmap()`. The standard malloc interface
provided by the libc is internally using these facilities to manage its
memory chunks.

Synchronization
===============

Klocks
------

The Zeke kernel provides various traditional synchronization primitives
for in kernel synchronization between accessors, multiple threads as
well as interrupt handlers.

### Mtx

Mtx is an umbrella for multiple more or less traditional spin lock types
and options. Main lock types are `MTX_TYPE_SPIN` implementing a very
traditional racy busy loop spinning and `MTX_TYPE_TICKET` implementing a
spin lock that guarantees the lock is given to waiters in they called
`mtx_lock()`, therefore per the most popular defintion `MTX_TYPE_TICKET`
can be considered as a fair locking method.

### Rwlock

Rwlock is a traditional readers-writer lock implementation using spin
locks for the actual synchronization between accessors.

### Cpulock

Cpulock is a simple wrapper for `mtx_t` providing a simple per CPU
locking method. Instead of providing somewhat traditional per CPU giant
locks, cpulock provides `cpulock_t` objects that can be created as need
basis.

### Index semaphores

Index semaphores is a Zeke specific somewhat novel concept for
allocating concurrently accessed indexes from an array. The acquistion
of indexes is blocking in case there is no indexes available, ie. the
`isema_acquire()` blocks until a free index is
found.

<span data-acronym-label="rcu" data-acronym-form="singular+full">rcu</span>
===========================================================================

<span data-acronym-label="rcu" data-acronym-form="singular+full">rcu</span>
is a low overhead synchronization method mainly used for synchronization
between multiple readers and rarely occuring writers.
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
can be considered to be a lightweight versioning system for data
structures that allows readers to access the old version of the data as
long as they hold a reference to the old data, while new readers will
get a pointer to the new version.

The
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
implementation supports only single writer and multiple readers by
itself but the user may use other synchronization methods to allow
multiple writers. Unlike traditional implementations the
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
implentation in Zeke allows interrupts and sleeping on both sides.
Instead of relying on time-based grace periods the
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
state is changed only when necessary due to a write synchronization. The
implementation also supports timer based reclamation of resources
similar to the traditional callback API. In practice the algorithm is
based on atomically accessed clock variable and two counters.

Before going any further with describing the implementation, let’s
define the terminology and function names used in this chapter

  - `gptr` is the global pointer referenced by both, readers and
    writer(s),

  - `rcu_assing_pointer()` writes a new value to a `gptr`,

  - `rcu_dereference_pointer()` dereferences the value of a `gptr`,

  - `rcu_read_lock()` increments the number of readers counter,

  - `rcu_read_unlock()` decrements the number of readers counter,

  - `rcu_call()` register a callback to be called when all readers of
    the old version of a `gptr` are ready, and

  - `rcu_synchronize()` block until all current readers are ready.

The global control variables for
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
are defined as follows

  - `clk` is a one bit clock selecting the
    <span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
    state,

  - `ctr0` is a counter for readers accepted in the first state,

  - `ctr1` is a counter for readers accepted in the second state, and

  - `rcu_reclaim_list[2]` contains a list of callbacks per state to old
    the versions of the resources.

In the following description we assume that the
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
is in the same state as initially but it has already ran an undefined
number of iterations, therefore \(\mathtt{clk} = 0\).

Both states work equally and the clock just swaps the functionality. In
the initial state `ctr0` is incremented on every call to
`rcu_read_lock()`. Whereas `rcu_read_unlock()` always decrements the
same counter that the previous call to `rcu_read_lock()` incremented,
this is ensured by passing a state variable to the caller when
`rcu_read_lock()` returns. After acquiring the lock a reader may call
`rcu_dereference_pointer()` to get the value pointed by a `gptr`.

The writer may call `rcu_assing_pointer()` to assing a new value to the
`gptr`. After the pointer has been assigned the writer should call
either `rcu_call()` or  
`rcu_synchronize()` to ensure there is no more readers for the old data,
and to free the old data. In general the latter is preferred if the
writer is allowed to block.

`rcu_synchronize()` is the only mechanism that will advance the global
<span data-acronym-label="rcu" data-acronym-form="singular+abbrv">rcu</span>
clock variable. The function works in two stages and access to the
function is syncronized by a spinlock. In the first stage it waits until
there is no more readers on the second counter (`ctr1`), and changes the
clock state. The second stage waits until there is no more readers on
the counter pointed by the previous clock state, in the initial case
this means `ctr0`. When `ctr0` has reached zero the reclamation of
resources registered to the reclaim list of the first state can be
executed. Note that these resources are not the ones that were assigned
to `gptr` pointers when the clock was in zero state but the resources
that were alloced when the clock was previously in the current state set
in the previous stage of `rcu_synchronize()`.

Process and Thread Management
=============================

Processes and Threads
---------------------

The process and thread management in Zeke is based on an idea that a
process is a container for threads owned by the process and it manages
the memory space and resources used by its threads. Naturally this means
that a process must always own at least one thread, that is the main
thread and all other threads are children of the main thread. While a
process is an unit or a context of resource allocation, a thread is an
object describing the execution state, in other words a unit of
execution, of an user space application. Any process not having a main
thread is immediately removed from the system because there is nothing
to execute.

Since a process is a container containing the main thread, and other
threads, there is no global execution state for a process, like in some
other operating systems. Some operating systems may use the main thread
status as the process state but Zeke makes an isolation between the
process state and the thread state. Thus a currently running process is
mostlikely in `READY` state for most of the time.

### A thread

#### The thread management concept

\center

![Thread tree
example.<span label="figure:thtree"></span>](gfx/proc-classdiag)

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

\center

![Process
states.<span label="figure:procstates"></span>](gfx/proc-states)

Figure [\[figure:procstates\]](#figure:procstates) shows every possible
state transition of a process in Zeke.

Calling `fork` syscall causes a creation of a new process container,
cloning of the currently executing thread as a new main thread as well
as marking all current memory regions as copy-on-write regions for both
processes. Marking all currently mapped regions as COW makes it easy to
get rid of otherwise hard to solve race conditions.

When a process calls `exec` syscall the current main thread is replaced
by a newly created main thread that’s pointing to the new process image,
ie.
<span data-acronym-label="PC" data-acronym-form="singular+abbrv">PC</span>
is set to the entry point of the new image. Before `exec` transfers
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

\center

![Thread states in the
scheduler.<span label="figure:threadstates"></span>](gfx/thread-states)

\center

![Scheduler CPU objects for two processor cores and per scheduler object
scheduling policy objects in priority
order.<span label="figure:objscheds"></span>](proc/objscheds)

Executable File Formats
-----------------------

### Introduction to executables in Zeke

The kernel has a support for implementing loader functions for any new
executable format but currently only 32bit
<span data-acronym-label="elf" data-acronym-form="singular+full">elf</span>
support exist. Loading of a new process image is invoked by calling exec
syscall call that calls `load_image()` function in `kern/exec.c` file.
Process image loaders are advertised by using `EXEC_LOADER` macro.

### ELF32

The
<span data-acronym-label="elf" data-acronym-form="singular+abbrv">elf</span>
loader in Zeke can be used to load statically linked executables as well
as anything linked dynamically. Currently only two loadable sections can
be loaded, code region and heap
region.

#### Suported <span data-acronym-label="elf" data-acronym-form="singular+abbrv">elf</span> sections

The kernel reads process memory regions based on information provided by
`PT_LOAD` sections in the
<span data-acronym-label="elf" data-acronym-form="singular+abbrv">elf</span>
file.

The kernel can read additional information needed for executing a binary
from the elf notes. The non-standard notes are only parsed if `Zeke` is
defined as an owner of the note.

`NT_VERSION` TODO

`NT_STACKSIZE` TODO

`NT_STACKIZE` note can be used to hint the kernel about the preferred
minimum size for the main thread stack.

`NT_CAPABILITIES` TODO

`NT_CAPABILITIES` note is used to inform the kernel about capabilities
required to execute a binary file. The elf loader attempts to set each
capability as an effective capability, which may fail if the capability
isn’t in the bounding capabilities set. In case the file has
+OunderscoreEXECunderscoreALTPCAP+ oflag set then the loader will first
add the capabilities found in these notes to the bounding capabilities
set, i.e. the executable can gain the bounding capabilities.

`NT_CAPABILITIES_REQ` TODO

`NT_CAPABILITIES__REQ` note functions similar to `NT_CAPABILITIES` but
it doesn’t allow new bounding capabilities to be gained even when the
binary file is opened with +OunderscoreEXECunderscoreALTPCAP+.

The user space implementation of these note types is discussed in
section
[1](#sec:libc_elf).

Libc: Process and Thread Management
-----------------------------------

### <span data-acronym-label="elf" data-acronym-form="singular+abbrv">elf</span> support

Zeke libc supports adding simple note sections to
<span data-acronym-label="elf" data-acronym-form="singular+abbrv">elf</span>
files by using macros provided by `sys/elf_notes.h` header file. As
discussed previously in chapter [](#chap:exec), some additional
information about the runtime environment requirements can be passed via
note sections. For example, a macro called `ELFNOTE_STACKSIZE` can be
used for indicating the minimum stack size needed by the main tread of
an executable, see listing [\[list:hugestack\]](#list:hugestack).

Another important Zeke specific note type is `NT_CAPABILITIES` that can
be used to annotate the capabilities required to succesfully execute a
given elf binary file. The capability notes can be created using the
`ELFNOTE_CAPABILITIES(...)` macro. Each note can have 64 capabilities
and the total number of these notes is unlimited. Depending on the file
system if the file is marked as a system file the binary can gain these
capabilities as bounding and effective capabilities, similar to +suid+
allowing a program to gain priviliges of another user.

The `NT_CAPABILITIES_REQ` note type is similar to `NT_CAPABILITIES` but
it doesn’t allow gaining new bounding capabilities. This note can be
created using `ELFNOTE_CAPABILITIES_REQ(...)` macro.

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

\epigraph{Tis in my memory lock'd,\newline
          And you yourself shall keep the key of it.}{\textit{Hamlet}, 1, ii}

System Calls
============

Introduction to System Calls
----------------------------

System calls are the main method of communication between the kernel and
user space applications. The word syscall is usually used as a shorthand
for system call in the context of Zeke and this book.

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

This chapter discusses only those system calls that does not fit to any
other part of this book.

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

IPC
===

Pipes
-----

Zeke supports unidirectional
<span data-acronym-label="POSIX" data-acronym-form="singular+short">POSIX</span>
pipe
<span data-acronym-label="ipc" data-acronym-form="singular+short">ipc</span>.
The implementation inherits nofs and it utilizes `queue_r` to provide
lockess data transfer between a producer and a consumer. However, the
current implementation doesn’t guarantee the atomicity the requirement
set by
<span data-acronym-label="POSIX" data-acronym-form="singular+short">POSIX</span>.
POSIX requires a transaction under `PIPE_BUF` bytes to be atomic.

Pseudo Terminals
----------------

A pseudo terminal is a interprocess communication channel that acts like
a terminal and looks like terminal. One end of the channel is the master
end and one end is the slave side of the channel. Pseudo terminals can
be used to implement terminal emulation functionality that looks like an
ordinary user terminal over
tty.

shmem
-----

<span data-acronym-label="shmem" data-acronym-form="singular+full">shmem</span>
is a part of the
<span data-acronym-label="ipc" data-acronym-form="singular+short">ipc</span>
subsystem providing shared memory between processes and also to some
device drivers like frame buffers.

The userland interface to shmem is provided by `sys/mman.h` and it’s
described in libc part of this book as well as in corresponding man
pages.

Signals
-------

The signal system is implemented to somewhat comfort with
<span data-acronym-label="POSIX" data-acronym-form="singular+abbrv">POSIX</span>
threads and process signaling. This means that signals can be sent to
both processes as well as threads. Signals are passed to signal
receiving entities in forwarding fashion, a bit like how packet
forwarding could work.

### Userspace signal handlers

Figure [\[figure:sigstack\]](#figure:sigstack) shows how entry to a user
space signal handler constructs necessary data on top of the user stack
and alters the thread stack frame.

\center

![A signal stack and the new thread stack
frame.<span label="figure:sigstack"></span>](pics/signal_stack)

Figure [\[figure:syscallint\]](#figure:syscallint) shows the signal
handling flow when a signal is received in a middle of a system call and
the system call is interrupted to immediately run the user space signal
hanlder.

\center

![Interruption of a system call when a signal is
caught.<span label="figure:syscallint"></span>](gfx/syscallint)

File System
===========

File System Abstraction
-----------------------

### General principles

In this part of the documentation terms
<span data-acronym-label="vnode" data-acronym-form="singular+abbrv">vnode</span>
and
<span data-acronym-label="inode" data-acronym-form="singular+abbrv">inode</span>
are sometimes used interchangeably. Historically inode was a way to
index files in the Unix file system and like. While Zeke continues to
use this terminology it adds a concept of a vnode that is used as an
abstraction level between actual the file systems and the
<span data-acronym-label="vfs" data-acronym-form="singular+full">vfs</span>.
This is implemented mostly the same way as in most of modern Unices
today.

Another important term often used in the context of Zeke’s
<span data-acronym-label="vfs" data-acronym-form="singular+abbrv">vfs</span>
is the superblock, it’s roughly an abstraction of an accessor to the
meta data or allocation table(s) of a physical (or virtual) file system.
For example for FAT this means that every volume has a superblock in
Zeke and the superblock can be used to access the file allocation table
and other label data of that volume.

In Zeke vnodes are always used as the primary access method to the files
in a file system, inodes are only accessed internally within the file
system implementation code if the file system supports/uses inodes.
Expected behavior is that a vnode and a vnode number exist in the system
as long as any process owns a reference to that vnode. After there are
no more references to a vnode it’s not guaranteed that the vnode exist
in memory and it cannot be retrieved by its vnode number. In fact
normally vnodes can’t be accessed by their vnode number but a vnode
number is guaranteed to be unique within a single file system,
superblock + vnode, but same vnode might be in use within another file
system and the number can be also reused.

To help understanding how
<span data-acronym-label="vfs" data-acronym-form="singular+abbrv">vfs</span>
works in Zeke, it can be simplistically described as an object storage
where file objects are first searched, found and the associated (open)
with a process. When a file is opened the process owns a pointer to the
file descriptor that contains some state information and pointer to the
actual file vnode. The vnode of the file is itself an object that knows
where contents of the file is stored (physical file system, superblock
pointer) and who knows how to manipulate the data (pointer to the vnode
operations struct). In fact vnode number itself is pretty much redundant
and legacy information for Zeke that is only provided for compatibility
reasons, the actual access method is always by a pointer reference to an
object.

### Kernel interface

The kernel interface to the actual file system drivers and file system
superblocks is built around virtual function structs defined in
<span data-acronym-label="vfs" data-acronym-form="singular+abbrv">vfs</span>
header file `fs.h` and some user space header files defining unified
data types.

A new file system is first registered to the kernel by passing a pointer
to fs struct that is a complete interface for mounting a new superblock
and interacting with the file system (note the difference between a file
system (driver) and a file system superblock that is referencing to the
actual data storage, while fs driver is accessing the superblock. The
file system struct is shown in [\[list:fs\]](#list:fs).

When superblock is mounted a superblock struct pointer is returned. This
pointer servers as the main interface to the newly mounted file system.
Superblock is defined as in listing [\[list:fs\_sb\]](#list:fs_sb). By
using superblock function calls it’s possible to get direct references
to vnodes and
<span data-acronym-label="vnode" data-acronym-form="singular+abbrv">vnode</span>
operations e.g. for modifying file contents or adding new hard links to
a directory node.

```c
/**
 * File system.
 */
typedef struct fs {
    char fsname[8];

    struct fs_superblock * (*mount)(const char * mpoint, uint32_t mode,
            int parm_len, char * parm);
    int (*umount)(struct fs_superblock * fs_sb);
    struct superblock_lnode * sbl_head; /*!< List of all mounts. */
} fs_t;
```

```c
/**
 * File system superblock.
 */
typedef struct fs_superblock {
    fs_t * fs;
    dev_t dev;
    uint32_t mode_flags; /*!< Mount mode flags */
    vnode_t * root; /*!< Root of this fs mount. */
    char * mtpt_path; /*!< Mount point path */

    /**
     * Delete a vnode reference.
     * Deletes a reference to a vnode and destroys the inode corresponding to the
     * inode if there is no more links and references to it.
     * @param[in] vnode is the vnode.
     * @return Returns 0 if no error; Otherwise value other than zero.
     */
    int (*delete_vnode)(vnode_t * vnode);
} fs_superblock_t;
```

### VFS hash

`vfs_hash` is a hashmap implementation used for vnode caching of
physically slow file systems. VFS itself doesn’t use this caching for
anything, and thus compiling `vfs_hash` is optional but file systems are
allowed to require it.

\[FAT\]<span>File Allocation Table</span>

fatfs
-----

### Overall description

Fatfs driver is an implementation of
<span data-acronym-label="fat" data-acronym-form="singular+full">fat</span>
file system in
    Zeke.

**Main features**

  - <span data-acronym-label="fat" data-acronym-form="singular+abbrv">fat</span>12/16/32,
  - Supports multiple ANSI/OEM code pages including DBCS,
  - <span data-acronym-label="lfn" data-acronym-form="singular+full">lfn</span>
    support in ANSI/OEM or Unicode.

Note that Microsoft Corporation holds a patent for
<span data-acronym-label="lfn" data-acronym-form="singular+short">lfn</span>
and commercial use might be prohibitten or it may require a license form
the company. Therefore it’s also possible to disable
<span data-acronym-label="lfn" data-acronym-form="singular+abbrv">lfn</span>
support in Zeke if required.

procfs
------

TODO

ramfs
-----

### Overall description

The purpose of the
<span data-acronym-label="ramfs" data-acronym-form="singular+abbrv">ramfs</span>
is to provide a storage in RAM for temporary files. This is important in
user scope where there might be no other writable storage medium
available on some embedded platform. In addition to storage for
temporary files in user scope ramfs could be useful to store kernel
files as well, e.g. init can be unpacked from kernel to ramfs and then
executed.

In the future ramfs code might be also useful for
<span data-acronym-label="vfs" data-acronym-form="singular+abbrv">vfs</span>
caching, so that changes to a slow medium could be stored in ramfs-like
structure and synced later.

Ramfs should implement the same
<span data-acronym-label="vfs" data-acronym-form="singular+abbrv">vfs</span>
interface as any other regular file system for Zeke. This ensures that
the same POSIX compliant file descriptor interface\[4\] can be used for
files stored to ramfs as well as to any other file system.

### Structure of ramfs

Data in Ramfs is organized by inodes,
<span data-acronym-label="inode" data-acronym-form="singular+abbrv">inode</span>
can contain either a file or directory entries, which means that an
inode can be either files or directories. Maximum size of a mounted
ramfs is limited by size of `size_t` type. Maximum size of a file is
limited by size of `off_t` type. Figure
[\[figure:inodes\]](#figure:inodes) shows a high level representation of
how inodes are organized and linked in ramfs to form directory tree with
files and directories.

Contents of a file is stored in blocks of memory that are dynamically
allocated on demand. Block size is selected per file so all blocks are
equal size and block pointer array (`in.data`) is expanded when new
blocks are allocated for a file. Figure [\[figure:file\]](#figure:file)
shows how memory is allocated for a file inode.

Directory in ramfs is stored similarly to a file but `in.dir` (union
with `in.data`) is now constant in size and it’s used as a hash table
which points to chains of directory entries (directory entry arrays). If
two files are mapped to a same chain by hash function then the
corresponding chain array is re-sized so that the new entry can be added
at the end of array. This means that lookup from a directory entry chain
is quite cache friendly and can usually avoid fragmentation in memory
allocation. Figure [\[figure:dir\]](#figure:dir) represents a directory
inode containing some directory entries.

\center

![Mounted ramfs with some
inodes.<span label="figure:inodes"></span>](pics/inodes)

\center

![Structure of a file stored in
ramfs.<span label="figure:file"></span>](pics/file)

\center

![Directory containing some directory entries in
ramfs.<span label="figure:dir"></span>](pics/dir)

### Time complexity and performance analysis

#### Mount

When a new ramfs superblock is mounted it’s appended to the end of the
global superblock list. Time complexity of this operation is \(O(n)\) as
there is no information about the last node in the list. However this is
not a major problem because new mounts happen relatively rarely and
other operations during the mounting process will take much more time
than traversing a super block list of any practical length. Those other
operations includes allocating memory for inode array, allocating memory
for inode pool and initializing some data structures etc.

#### Get vnode by vnode number

Index nodes are stored in contiguous array as shown in figure
[\[figure:inodes\]](#figure:inodes). This trivially means that a vnode
can be always fetched from mounted ramfs in \(O(1)\) time as it’s only a
matter of array indexing.

#### Lookup vnode by filename

File lookup on ramfs level is implemented only on single directory vnode
level and sub-directory lookup must be then implemented on
<span data-acronym-label="vfs" data-acronym-form="singular+abbrv">vfs</span>
level i.e. lookup function for a vnode can only lookup for entries in
the current directory vnode. A file lookup is made by calling
`vnode->vnode_ops->lookup()`.

Directory entries are stored in chained hash tables as shown in figure
[\[figure:dir\]](#figure:dir). It can be easily seen that usually a
chain array contains only a single entry and therefore lookup is
\(O(1)\). In sense a of performance and efficiency of a chain array with
length greater than zero is more complex. Trivially lookup from array is
of course \(O(n)\). What is interesting here is how CPU caching works
here. Even though directory entries are different in size they are all
stored in contiguous memory area and can be loaded to CPU cache very
efficiently, whereas many other data structures may lead to several
non-contiguous memory allocations that will pollute the caching and slow
down the lookup if there is only few entries. That being said the current
implementation of directory entry storage seems almost perfect solution
if amount of directory entries is moderate and hash functions is good
enough.

#### Data by vnode

Data stored in file inodes is accessed by calling
`vnode->vnode_ops->read()` and `vnode->vnode_ops->write()`. Arguments
for both contains a pointer to the vnode, offset where to start reading
or writing from, byte count and a pointer to a buffer. Data structuring
of a file vnode was illustrated in figure
[\[figure:file\]](#figure:file).

Pointer to a block of data by `offset` is calculated as shown in
equation [\[eqn:dpointer\]](#eqn:dpointer), where data is the data block
pointer array. Length of data pointer by the pointer is calculated as
shown in equation [\[eqn:dlen\]](#eqn:dlen).

\[\begin{aligned}
  \textrm{block} &=& \textrm{data} \left[ \frac{\textrm{offset} -
    (\textrm{offset} \& (\textrm{blksize} - 1))}{\textrm{blksize}}
    \right] \\
  \textrm{p}     &=& \textrm{block} \left[ \textrm{offset} \& (\textrm{blksize} - 1)
    \right] \label{eqn:dpointer} \\
  \textrm{len}  &=& \textrm{blocksize} - (\textrm{offset} \& (\textrm{blksize} - 1)) \label{eqn:dlen}\end{aligned}\]

#### Create a vnode

Normally when a new inode is created it can be taken from the inode pool
and inserted at empty location on inode array. Average case can be
considered to be \(O(1)\) then.

Lets consider a case where a new vnode is created and inserted to the
index node array but it’s already full. A call to `krealloc()` will be
issued and the worst case of reallocation of a memory location may
require to create a copy of the whole index node array to a new
location. This means that the worst case time complexity of a vnode
creation is relative to size of the array, so its
\(O(n)\).

#### Summary

| Function              | Average time complexity | Worst case time complexity |
| :-------------------- | :---------------------: | :------------------------: |
| mount                 |        \(O(n)\)         |          \(O(n)\)          |
| vnode by vnode number |        \(O(1)\)         |          \(O(1)\)          |
| vnode by filename     |        \(O(1)\)         |          \(O(n)\)          |
| data by vnode         |        \(O(1)\)         |          \(O(1)\)          |
| create a new vnode    |        \(O(1)\)         |          \(O(n)\)          |

Summary of time complexity of ramfs
functions.<span label="table:complexity"></span>

### Performance testing

Automated performance tests were implemented the same way as unit tests.
Ramfs performance tests are in `test_ramfsperf.c` file.

#### Test results

#### Hard link operations

Performance of hard link operations is tested in `test_dehtableperf.c`.

Figure [\[figure:dhlink\_perf\]](#figure:dhlink_perf) shows performance
measurements from `test_link_perf()` test. In this test number of
randomly named nodes are added at every point and total time of adding
those links is measured. Name collisions are not handled but just
appended to the chain array. It seems that total time of `dh_link()`
calls is almost flat but it then starts to smoothly transform to
something that is almost linearly relative to the amount of links added.
Calls to `kmalloc()` and `krealloc()` seems to add some random behavior
to link times.

Lookup tests were performed with `test_lookup_perf()` test and
illustrated in figure
[\[figure:dhlookup\_perf\]](#figure:dhlookup_perf). The test works by
first adding certain amount of randomly named links and then trying to
lookup with another set of randomly selected names. Hit percentage is
named as "% found" in the plot and lookup time is the mean of 100 random
lookups. Lookup function seems to behave quite linearly even though
there is some quite strange deviation at some link counts.

Even though lookups seems to be in almost linear relationship with the
link count of the directory entry hash table it doesn’t mean lookups are
slow. Even with 20000 links average lookup takes only \(45\:\mu s\) and
it’s very rare to have that many directory entries in one directory.

![Directory entry hash table link
performance.](plots/dh_link)

<span id="figure:dhlink_perf" label="figure:dhlink_perf">\[figure:dhlink\_perf\]</span>

![Directory entry hash table lookup
performance.](plots/dh_lookup)

<span id="figure:dhlookup_perf" label="figure:dhlookup_perf">\[figure:dhlookup\_perf\]</span>

##### File operations

Performance tests for file operations were performed on the universal
target where kmalloc uses malloc instead of dynem block allocator, this
in fact may make some of the result unreliable.

| **Operation**             |      |    |
| :------------------------ | ---: | :- |
| **Single write (100 MB)** |      |    |
| new file                  |   21 | 62 |
| existing file             | 1694 | 92 |
| read                      | 1098 | 90 |
| **Sequential writes**     |      |    |
| new file                  |    9 | 06 |
| existing file             |  335 | 80 |
| read                      |  426 | 22 |

##### ramfs write/read performance

Write and read performance testing is somewhat biased by the underlying
kernel even though we have our own memory allocator in place. Actually
kmalloc may make things work even worse because it’s optimized for
different kind of suballocator that is not present on API level of
Linux. So I think this is a major cause for very poor memory allocation
and first pass write performance to the allocated blocks of memory,
although kmalloc it self is quite inefficient too.

### Suggestions for further development

#### Directories

Directory entry lookup tables (hash tables) could be made variable in
size. This would make use of directory entry chains less frequent and
greatly improve performance of large directories while still maintaining
small footprint for small directories containing only few entries. At
minimum this would only require a function pointer to the current hash
function in the inode. There is no need to store the size of current
hash table array in variable because it can be determined by comparing
the function pointer. So overhead of this improvement would be size of
`size_t` per directory and one more dereference per lookup.

devfs
-----

### Overal description

Devfs inherits ramfs and creates an abstraction layer between device
drivers and device driver abstraction layers. Essentially devfs creates
a file abstraction with vfs read() and write() functions that
communicates with the actual device driver.

### Device creation process

Device registration with devfs starts from static init function of a
subsystem, device detection routine or some other triggering method. The
device identification/creation function shall, by some way, create a
`dev_info` struct that describes the device and provides necessary
function pointers for reading and writing. Then
`make_dev(devXXX_info, 0, 0, 0666)` is called to register the device
created. `make_dev()` then creates a fs node that is contains a pointer
to the provided `dev_info` in `vn_specinfo` of the device.

There is some notable differences between devfs implementation of Zeke
and other common devfs or device abstractions in some other operating
systems, particularly Unices. First of all we don’t use majorminor
combination as a device idetentifier, it’s only provided for
compatibility reasons and not used for anything actually.\[5\] So
devices can’t be accessed by creating a device file anywhere in the
system, device files in Zeke are very special and only ones that are
created with `make_dev()` are valid, since the object oriented model of
Zeke VFS.

Another difference is that Zeke does not have character and block device
access modes/file types like most of traditional Unices. Zeke can
support buffered writes but it’s always hidden from the user space. In
practice, it means that every user reading from a device file will
always see the same state, eg. if one process would write by using block
device and another one reading character device, the latter one would
get either old data or corrputed data. In fact there is no reason to
have different file types for different device types, block device files
were designed to be a special file type for hard disks, for some reason,
but it doesn’t make any sense to do it that way in a modern kernel. Thus
block device files are not supported in Zeke and buffered access is not
implemented but technically supported inside the kernel.

- [Poul-Henning Kamp - Rethinking /dev and devices in the unix kernel](https://www.usenix.org/legacy/events/bsdcon/full_papers/kamp/kamp_html/)

### UART devices

UART devices are handled by UART submodule (`kern/hal/uart.c`) so that
common code between UART implementations can be shared and tty numbering
can be centrally organized. To register a new UART port the device
driver has to allocate a new `uart_port` structure and pass it to
`uart_register_port()` which finally registers the a new device file for
the port.

\center

![Communication between subsystems when a user process is writing to a
UART.<span label="figure:fsuart"></span>](pics/uart)

Build System
============

Kernel Build
------------

`kern/Makefile` is responsible of compiling the kernel and parsing kmod
makefiles. Module makefiles are following the naming convention
`<module>.mk` and are located under `kern/kmodmakefiles` directory.

Module makefiles are parsed like normal makefiles but care should be
taken when changing global variables in these makefiles. Module
makefiles are mainly allowed to only append `IDIR` variable and all
other variables should be more or less specific to the module makefile
itself and should begin with the name of the module.

An example of a module makefile is shown in listing
[\[list:modulemk\]](#list:modulemk).

```makefile
# Module example
# Mandatory file
# If any source files are declared like this the whole module becomes
# mandatory and won't be dropped even if its configuration is set to zero.
module-SRC-1 += src/module/test.c
# Optional file
# If all files are declared like this then the module is optional and can be
# enabled or disabled in the `config.mk`.
module-SRC-$(configMODULE_CONFIGURABLE) += src/module/source.c
# Assembly file
module-ASRC$(configMODULE_CONFIGURABLE) += src/module/lowlevel.S
```

The kernel makefile will automatically discover `test-SRC-1` list and
will compile a new static library based on the compilation units in the
list. Name of the library is derived from the makefile’s name and so
should be the first word of the source file list name.

Userland Build
--------------

Userland makefiles are constructed from `user_head.mk`, `user_tail.mk`
and the actual targets between includes. A good example of a user space
makefile is `bin/Makefile` that compiles tools under `/bin`. A manifest
file is automatically generated by the make system and it will be used
for creating a rootfs image with `tools/mkrootfs.sh` script.

Libraries
---------

TODO

Applications
------------

TODO

Tools Build
-----------

TODO

Testing
=======

PUnit
-----

PUnit is a portable unit test framework for C, it’s used for userland
testing in Zeke, mainly for testing libc functions and testing that some
kernel system call interfaces work properly when called from the user
space.

### Assertions

  - `pu_assert(message, test)` - Checks if boolean value of test is true

  - `pu_assert_equal(message, left, right)` - Checks if `left == right`
    is true

  - `pu_assert_ptr_equal(message, left, right)` - Checks if left and
    right pointers are equal

  - `pu_assert_str_equal(message, left, right)` - Checks if left and
    right strings are equal (strcmp)

  - `pu_assert_array_equal(message, left, right, size)` - Asserts that
    each integer element i of two arrays are equal (`strcmp()`)

  - `ku_assert_str_array_equal(message, left, right, size)` - Asserts
    that each string element i of two arrays are equal (`==`)

  - `pu_assert_null(message, ptr)` - Asserts that a pointer is null

  - `pu_assert_not_null(message, ptr)` - Asserts that a pointer isn’t
    null

  - `ku_assert_fail(message)` - Always fails

KUnit - Kernel Unit Test Framework
----------------------------------

KUnit is a PUnit based portable unit testing framework that is modified
to allow running unit tests in kernel space, called in-kernel unit
testing. This is done because it’s not easily possible to compile kernel
code to user space to link against with unit test code that could be run
in user space.

When kunit is compiled with the kernel tests are exposed in sysctl MIB
tree under debug.test. Any test can be invoked by writing a value other
than zero to the corresponding variable in MIB. While test is executing
the user thread is blocked until test tests have been run. Test results
are written at "realtime" to the kernel logger.

KUnit and unit tests are built into the kernel if `configKUNIT` flag is
set in config file. This creates a `debug.test` tree in sysctl
<span data-acronym-label="MIB" data-acronym-form="singular+short">MIB</span>.
Unit tests are executed by writing value other than zero to the
corresponding variable name in MIB. Tests results are then written to
the kernel log. Running thread is blocked until requested test has
returned.

### Assertions

  - `ku_assert(message, test)` - Checks if boolean value of test is true

  - `ku_assert_equal(message, left, right)` - Checks if `left == right`
    is true

  - `ku_assert_ptr_equal(message, left, right)` - Checks if left and
    right pointers are equal

  - `ku_assert_str_equal(message, left, right)` - Checks if left and
    right strings are equal (strcmp)

  - `ku_assert_array_equal(message, left, right, size)` - Asserts that
    each integer element i of two arrays are equal (`strcmp()`)

  - `ku_assert_str_array_equal(message, left, right, size)` - Asserts
    that each string element i of two arrays are equal (`==`)

  - `ku_assert_null(message, ptr)` - Asserts that a pointer is null

  - `ku_assert_not_null(message, ptr)` - Asserts that a pointer isn’t
    null

  - `ku_assert_fail(message)` - Always fails

\bibliographystyle{plain}

1.  Zeke is portable to at least in a sense of portability between
    different ARM cores and architectures.

2.  The project is aiming to implement the core set of kernel features
    specified in
    <span data-acronym-label="POSIX" data-acronym-form="singular+short">POSIX</span>
    standard.

3.  At least almost from scratch, some functionality is derived from BSD
    kernels as well as some libraries are taken from other sources.

4.  POSIX file API is not actually yet implemented.

5.  Some drivers may still use those internally but there is no external
    interface provided. Uart is one of those using minor numbers for
    internal indexing.
