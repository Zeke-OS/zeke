/**
 * File system superblock.
 */
typedef struct fs_superblock {
    fs_t * fs;
    dev_t dev;
    uint32_t mode_flags; /*!< Mount mode flags */
    vnode_t * root; /*!< Root of this fs mount. */
    char * mtpt_path; /*!< Mount point path */

    int (*get_vnode)(struct fs_superblock * sb, ino_t * vnode_num, vnode_t ** vnode);
    int (*delete_vnode)(vnode_t * vnode);
} fs_superblock_t;

