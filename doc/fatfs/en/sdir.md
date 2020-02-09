## DIR

The `DIR` structure is used for the work area to read a directory by
`f_oepndir()/f_readdir()` function. Application program must not modify
any member in this structure.

    typedef struct {
        FATFS*  fs;         /* Pointer to the owner file system object */
        WORD    id;         /* Owner file system mount ID */
        WORD    index;      /* Index of directory entry to start to search next */
        DWORD   sclust;     /* Table start cluster (0:Root directory) */
        DWORD   clust;      /* Current cluster */
        DWORD   sect;       /* Current sector */
        BYTE*   dir;        /* Pointer to the current SFN entry in the win[] */
        BYTE*   fn;         /* Pointer to the SFN buffer (in/out) {file[8],ext[3],status[1]} */
    #if _FS_LOCK
        UINT    lockid;     /* Sub-directory lock ID (0:Root directory) */
    #endif
    #if _USE_LFN
        WCHAR*  lfn;        /* Pointer to the LFN buffer (in/out) */
        WORD    lfn_idx;    /* Index of the LFN entris (0xFFFF:No LFN) */
    #endif
    } DIR;
