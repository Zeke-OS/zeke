

#define LLIST_TSLIST    0x0 /*!< Singly linked list. */
#define LLIST_TDLIST    0x1 /*!< Doubly linked list. */
#define LLIST_TCIRC     0x4 /*!< Circular linked list. */

typedef struct {
    struct llist * list_head;
    void * next;
    void * prev;
} llistssnode_t;

typedef struct {
    struct llist * list_head;
    void * next;
    void * prev;
} llist_dnode_t;

/**
 * Generic linked list.
 */
typedef struct llist {
    unsigned int type;
    size_t offset; /*!< Offset of node entry. */
    void (*add_after)(struct * llist list, void * node, void * new_node);
    void (*add_before)(struct * llist list, void * node, void * new_node);
    void (*remove)(struct * llist list, void * node);
    void (*remove_head)(struct * llist list, void * node);
    void (*remove_tail)(struct * llist list, void * node);
} llist_t;
