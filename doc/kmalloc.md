kmalloc
=======

Initial implementation of the generic kernel memory allocator is largely based
on tutorial written by Marwal Burrel [1]. However kmalloc doesn't exploit heap
space at all. Instead of using contiguous heap space all memory allocations are
done by allocating new memory regions for the kernel by using dynmem system.

For overall description of memory management in zeke see vmem.md.

Brief Description
-----------------

The current kernel memory allocator implementation is somewhat naive and
exploits some very simple techniques like the first fit algorithm for allocating
memory.

Idea of the first fit algorithm is to find a first large enough free block of
memory from an already allocated region of memory. This is done by traversing
the list of memory blocks and looking for sufficiently large block. This is
ofcourse quite suboptimal and better solutions has to be considered.

When a large enough block is found it's splitted in two halves so that the left
one corresponds to requested size and the right block is left free. All data
blocks are aligned to 4 byte access.

Fragmentation of memory blocks is tried to keep minimal by merging newly freed
block with neighbouring blocks. This will at least keep all free blocks between
reserved blocks free but it doesn't help much if there is allocations of
different sizes that are freed independently. So current implementation is very
suboptimal also in this area.

When kmalloc is out of (large enough) memory blocks it will expand its memory
usage by allocating a new region of memory from dynmem. Allocation is done in
1 MB blocks (naturally) and always rounded  to a next 1 MB.

    +-+------+-+---------+-+-------+
    |d|      |d|         |d|       |
    |e| DATA |e|  FREE   |e| DATA  |
    |s|      |s|         |s|       |
    |c|      |c|         |c|       |
    +-+------+-----------+-+-------+

Descriptors are used to store the size of the data block, reference counters and
pointers to neighbouring block descriptors.


References
----------

[1] http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf
