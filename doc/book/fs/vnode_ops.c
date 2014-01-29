const vnode_ops_t ramfs_vnode_ops = {
    .write = ramfs_write,
    .read = ramfs_read,
    .create = ramfs_create,
    .lookup = ramfs_lookup,
    .link = ramfs_link,
    .mkdir = ramfs_mkdir,
    .readdir = ramfs_readdir
};
