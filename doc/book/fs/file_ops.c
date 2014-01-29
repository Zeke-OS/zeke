/**
 * vnode operations struct.
 */
typedef struct file_ops {
    /* Normal file operations */
    int (*lock)(vnode_t * file);
    int (*release)(vnode_t * file);
    int (*write)(vnode_t * file, int offset, const void * buf, int count);
    int (*read)(vnode_t * file, int offset, void * buf, int count);
    /* Directory file operations */
    int (*create)(vnode_t * dir, const char * name, int name_len, vnode_t ** result);
    int (*mknod)(vnode_t * dir, const char * name, int name_len, int mode, dev_t dev);
    int (*lookup)(vnode_t * dir, const char * name, int name_len, vnode_t ** result);
    int (*link)(vnode_t * oldvnode, vnode_t * dir, const char * name, int name_len);
    int (*unlink)(vnode_t * dir, const char * name, int name_len);
    int (*mkdir)(vnode_t * dir,  const char * name, int name_len);
    int (*rmdir)(vnode_t * dir,  const char * name, int name_len);
    /* Operations specified for any file type */
    int (*stat)(vnode_t * vnode, struct stat * buf);
} vnode_ops_t;