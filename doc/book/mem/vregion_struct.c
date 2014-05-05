struct vregion {
    llist_nodedsc_t node;
    intptr_t paddr;
    size_t count;
    size_t size;
    bitmap_t map[0];
};