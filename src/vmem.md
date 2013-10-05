Virtual Memory in Zeke
======================

Every process has its own master page table while L2 page tables are shared.
Kernel has its own master page table too. Static/fixed entries are copied to all
master page tables created. Process shares its master page table with its
childs.

ARM note: Only 4 kB pages are used so XN (Execute-Never) bit is always supported
also for L2 pages.

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

