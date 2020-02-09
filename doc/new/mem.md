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
