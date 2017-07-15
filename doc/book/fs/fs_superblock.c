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
