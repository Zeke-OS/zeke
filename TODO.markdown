TODO
====

Generic Tasks
-------------

- Timer syscalls
- Optional auto-reset on failure instead of halt
- Implement system that calculates actual number of systicks consumed by
  the scheduler (and compare with others as well like for example idle task)
- Use hw optimization for bitmap_search()
- sysconf: http://pubs.opengroup.org/onlinepubs/7990989775/xsh/sysconf.html
- dlfcn.h - dynamic linking

The current "relaxed" memory layout is not sufficient anymore and it's painful
to manage. Starting user space memory from 0 would make user space development
a bit easier. Separated locked kernel memory area would greatly reduce overhead
of syscalls and make it trivial to know if some data is accessible or not.
Currently we have some assumptions that some data must exist in low memory and
those can't be relocated to maintain operational process switches. All of this
makes constant cache flushes a strict requirement and slows downs things.
Good things however are that the kernel could easily support non-MMU hw and
in-kernel memory allocations are not limited by any artificial limits.

proc
----

- sysctl kern.maxproc should not allow negative values
- Update vm_pt linkcount correctly during fork() and other operations
- proc is not MP safe
- exec(), elf
- kill()
- signals

fs (vfs)
--------

- lock to files struct
- syscalls missing
    - dup
    - fsstat
    - umount
    - mmap

ramfs
-----

Allocate blocks with geteblk, this will also make memmapping possible for ramfs
files.
Use LINK_MAX.

kinit
-----

- Print something when a subsys init begins

sched
-----

Implement thread cancellation in a POSIX way.

Zombie threads are not cleaned properly.

Short sched_thread_sleep() delays may crash the kernel. This may happen because
of some race condition with scheduling. So times under 5 ms in some conditions
overflow in scheduler heap.

*Time slice/quantum accounting fairness*
Preserve all or some unused time slices when thread is put back on exec
after sleep.

*Use process priority as a base priority*
Process priorities could be scaled to something like 100 or 10 and the thread
priority is added to that value to get the effective scheduling priority.

*Signals are also needed for threads*
For example sigsegv is passed usually directly to a thread that caused it,
instead of the process, according to POSIX2013.

*MP support, there can't be currrent_thread global variable on MP system*
Either remove it by storing the information in thread stack OR have a CPU
specific variable/array.

- Priority reward metrics for IO bound processes.
- Calculate thread total execution time.
- ARM timer to systimer, should improve accuracy
- Use thread parent<->client relationship to improve scheduling

HAL
---

- thread stack dump code and kill code

interrupt
---------

perf
----

- perf events to sysctl
- perf event counting for page faults
- Implement perf events code for ARM11

MMU
---

- Instead of always flushing everything flush just addresses changed if
  possible
- XN bit may not work
- Better debug messages to detect invalid page tables/regions
- Individual locks for page tables.
  May improve performance and reliability but might be hard to implement.

ptmapper
--------

- Add kbrk() and ksbrk() functions to allocate empty space of kern data section
- Allocate page tables from dynmem after pt area is consumed.
  This should work just fine but there is some alignment things so we should
  have dedicated allocator for hw page tables, we could possibly use vralloc to
  get things right and avoid duplicated code.
- throwaway (coarse) page table support
- Allocate coarse page table sets, Contiguous set of coarse page tables
- add locks
- refactor static structs into array or btree

dynmem
------

- COW may cause deadlock if dynmem is called

kmalloc
-------

- use new kbrk
- COW may cause deadlock
- add locks

vralloc
-------

- More locks to protect global data

vm
--

*vm_automap_region() function to automatically map a buffer to any free location*

*vm_map_region() should call allocator specific mapper*
So for VRA this call would just call mmu_map_region() but for some COR mapper
it would possibly allocate only requested page and not the whole region and for
secondary storage mapper it would be just a stub.

*COR - Copy On Read flag*
Could be used for paging memory allocator?

*vm_region struct should contain mmu as union*
...so data can be located in some other location mmu in linked memory
(eg. swapped).

*Virtual copyin() and copyout()*
Instead of actualy copyin/out we could just map data to somewhere

*Handle acc PROT_COW*
Should possibly make a copy on the fly if wr is requested.

klocks
------

*semaphore*
A main use for semaphore would be mutex that can sleep/switch context instead of
spinning. This should have a queue of threads waiting for this lock (in kernel
mode).

*mtx_irq*
Gets lock and disable interrupts on current cpu. Should be added as a type and
then add function spinlock_irq() and/or spinlock_irqsave().

"sleep lock" implementation of rwlocks? or some combination of both?

kerror
------

- log read functions

CRT
---

- vfp and sfp support

libc
----

- Missing this and that
- Enable mmap for malloc
- sigtimedwait, sigwaitinfo implementations missing

init
----

- fork() new processes like init should do


KNOWN ISSUES
============


NOTES
=====

