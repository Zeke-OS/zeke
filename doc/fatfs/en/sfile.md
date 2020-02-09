## FIL

The `FIL` structure (file object) holds state of an open file. It is
created by `f_open()` function and discarded by `f_close()` function.
Application program must not modify any member in this structure except
for `cltbl`. Note that a sector buffer is defined in this structure at
non-tiny configuration, so that the `FIL` structures should not be
defined as auto variable.

```c
typedef struct {
    FATFS*  fs;           /* Pointer to the owner file system object */
    WORD    id;           /* Owner file system mount ID */
    BYTE    flag;         /* File object status flags */
    BYTE    err;          /* Abort flag (error code) */
    DWORD   fptr;         /* File read/write pointer (Byte offset origin from top of the file) */
    DWORD   fsize;        /* File size in unit of byte */
    DWORD   sclust;       /* File start cluster */
    DWORD   clust;        /* Current cluster */
    DWORD   dsect;        /* Current data sector */
#if !_FS_READONLY
    DWORD   dir_sect;     /* Sector containing the directory entry */
    BYTE*   dir_ptr;      /* Ponter to the directory entry in the window */
#endif
#if _USE_FASTSEEK
    DWORD*  cltbl;        /* Pointer to the cluster link map table (Nulled on file open) */
#endif
#if _FS_LOCK
    UINT    lockid;       /* Fle lock ID */
#endif
#if !_FS_TINY
    BYTE    buf[_MAX_SS]; /* File private data transfer buffer */
#endif
} FIL;
```
