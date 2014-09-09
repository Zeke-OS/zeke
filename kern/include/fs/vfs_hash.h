#ifndef MSDOSFS_HASH_H
#define MSDOSFS_HASH_H

struct vnode;

typedef int vfs_hash_cmp_t(struct vnode * vp, void * arg);

int vfs_hash_get(const struct fs_superblock * mp, unsigned hash,
                 struct vnode ** vpp, vfs_hash_cmp_t * fn, void * arg);
unsigned vfs_hash_index(struct vnode * vp);
int vfs_hash_insert(struct vnode * vp, unsigned hash,
                    struct vnode ** vpp, vfs_hash_cmp_t * fn, void * arg);
void vfs_hash_rehash(struct vnode * vp, unsigned hash);
void vfs_hash_remove(struct vnode * vp);

#endif /* MSDOSFS_HASH_H */
