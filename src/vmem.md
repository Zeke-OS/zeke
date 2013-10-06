Virtual Memory in Zeke
======================

Every process has its own master page table and varying number of L2 page
tables. Kernel has its own master page table too. Static/fixed entries are
copied to all master page tables created. Process shares its master page
table with its childs.

Process page tables are stored in dynmem area.

ARM note: Only 4 kB pages are used with L2 page tables so XN (Execute-Never) bit
is always usable also for L2 pages.

Domains
-------

See MMU_DOM_xxx definitions.

Memory regions
--------------

Table: Static memory region placement

    Region      AP  Validity    Region  Virtual base    No. pages   Physical
                                size    address                     base address
    ----------------------------------------------------------------------------
    Kernel          fixed       32MB    0x0                         0x0
    Page table      fixed               0x2000000                   0x2000000


Virtual memory abstraction levels
---------------------------------

        +---------------+
        |  pmalloc(?)   |
        +---------------+
                |
        +---------------+
        |  kmalloc      |
        +---------------+
                |
        +---------------+
        |    dynmem     |
        +---------------+
                |
        +---------------+
        |    mmu HAL    |
        +---------------+
                |
        +-----------------------+
        | CPU specific MMU code |
        +-----------------------+
    ------------|-------------------
        +-------------------+
        | MMU & coProcessor |
        +-------------------+

+ kmalloc - is a kernel level memory allocation service.
+ dynmem - is a dynamic memory allocation system that allocates & frees
  contiguous blocks of physical memory.
+ mmu HAL - is the abract MMU interface provided by mmu.h & mmu.c
+ CPU specific MMU code is the module responsible of configuring the
  physical MMU layer and implementing the interface prodived by mmu.c


dynmem
------

Dynmem allocates 1MB sections from L1 kernel master page table and always
returns a physically contiguous memory region. If dynmem is passed for a thread
it can be mapped either as a section entry or via L2 page table.
