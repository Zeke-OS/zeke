/**
 * File system.
 */
typedef struct fs {
    char fsname[8];

    struct fs_superblock * (*mount)(const char * mpoint, uint32_t mode,
            int parm_len, char * parm);
    int (*umount)(struct fs_superblock * fs_sb);
    struct superblock_lnode * sbl_head; /*!< List of all mounts. */
} fs_t;
