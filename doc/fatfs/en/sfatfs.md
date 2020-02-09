## FATFS

The `FATFS` structure (file system object) holds dynamic work area of
individual logical drives. It is given by application program and
registerd/unregisterd to the FatFs module with `f_mount()` function.
Initialization is done on first API call after `f_mount()` function or
media change. Application program must not modify any member in this
structure.

```c
typedef struct {
    BYTE    fs_type;      /* FAT sub-type (0:Not mounted) */
    BYTE    drv;          /* Physical drive number */
    BYTE    csize;        /* Sectors per cluster (1,2,4,...,128) */
    BYTE    n_fats;       /* Number of FAT copies (1,2) */
    BYTE    wflag;        /* win[] flag (b0:win[] is dirty) */
    BYTE    fsi_flag;     /* FSINFO flags (b7:Disabled, b0:Dirty) */
    WORD    id;           /* File system mount ID */
    WORD    n_rootdir;    /* Number of root directory entries (FAT12/16) */
#if _MAX_SS != _MIN_SS
    WORD    ssize;        /* Sector size (512,1024,2048 or 4096) */
#endif
#if _FS_REENTRANT
    _SYNC_t sobj;         /* Identifier of sync object */
#endif
#if !_FS_READONLY
    DWORD   last_clust;   /* FSINFO: Last allocated cluster */
    DWORD   free_clust;   /* FSINFO: Number of free clusters */
#endif
#if _FS_RPATH
    DWORD   cdir;         /* Current directory start cluster (0:root) */
#endif
    DWORD   n_fatent;     /* Number of FAT entries (== Number of clusters + 2) */
    DWORD   fsize;        /* Sectors per FAT */
    DWORD   volbase;      /* Volume start sector */
    DWORD   fatbase;      /* FAT area start sector */
    DWORD   dirbase;      /* Root directory area start sector (FAT32: Cluster#) */
    DWORD   database;     /* Data area start sector */
    DWORD   winsect;      /* Current sector appearing in the win[] */
    BYTE    win[_MAX_SS]; /* Disk access window for directory, FAT (and file data at tiny cfg) */
} FATFS;
```
