typedef struct mblock {
    size_t size;            /* Size of data area of this block. */
    struct mblock * next;   /* Pointer to the next memory block desc. */
    struct mblock * prev;   /* Pointer to the previous memory block desc. */
    int refcount;           /*!< Ref count. */
    void * ptr;             /*!< Memory block descriptor validation.
                             *   This should point to the data. */
    char data[1];
} mblock_t;