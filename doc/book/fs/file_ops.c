/**
 * vnode operations struct.
 */
typedef struct vnode_ops {
    /* Normal file operations
     * ---------------------- */
    int (*lock)(vnode_t * file);
    int (*release)(vnode_t * file);
    size_t (*write)(vnode_t * file, const off_t * offset,
            const void * buf, size_t count);
    size_t (*read)(vnode_t * file, const off_t * offset,
            void * buf, size_t count);
    //int (*mmap)(vnode_t * file, !mem area!);
    /* Directory file operations
     * ------------------------- */
    int (*create)(vnode_t * dir, const char * name, size_t name_len,
            vnode_t ** result);
    int (*mknod)(vnode_t * dir, const char * name, size_t name_len, int mode,
            dev_t dev);
    int (*lookup)(vnode_t * dir, const char * name, size_t name_len,
            vnode_t ** result);
    int (*link)(vnode_t * dir, vnode_t * vnode, const char * name,
            size_t name_len);
    int (*unlink)(vnode_t * dir, const char * name, size_t name_len);
    int (*mkdir)(vnode_t * dir,  const char * name, size_t name_len);
    int (*rmdir)(vnode_t * dir,  const char * name, size_t name_len);
    int (*readdir)(vnode_t * dir, struct dirent * d);
    /* Operations specified for any file type */
    int (*stat)(vnode_t * vnode, struct stat * buf);
} vnode_ops_t;
