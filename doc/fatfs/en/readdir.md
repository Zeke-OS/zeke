## f\_readdir

The f\_readdir function reads directory entries.

    FRESULT f_readdir (
      DIR* dp,      /* [IN] Directory object */
      FILINFO* fno  /* [OUT] File information structure */
    );

#### Parameters

  - dp  
    Pointer to the open directory object.
  - fno  
    Pointer to the file information structure to store the read item.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm),
[FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

The `f_readdir()` function reads directory items, file and directory, in
sequence. All items in the directory can be read by calling
`f_readdir()` function repeatedly. When relative path feature is enabled
(`_FS_RPATH >= 1`), dot entries ("." and "..") are not filtered out and
they will appear in the read items. When all directory items have been
read and no item to read, a null string is returned into the `fname[]`
without any error. When a null pointer is given to the `fno`, the read
index of the directory object is rewinded.

When LFN feature is enabled, `lfname` and `lfsize` in the file
information structure must be initialized with valid value prior to use
it. The `lfname` is a pointer to the LFN read buffer. The `lfsize` is
size of the LFN read buffer in unit of `TCHAR`. If the LFN is not
needed, set a null pointer to the `lfname` and the LFN is not returned.
A null string will be returned into the LFN read buffer in case of
following conditions.

  - The directory item has no LFN information.
  - Either the size of read buffer or LFN working buffer is insufficient
    for the LFN.
  - The LFN contains any Unicode character that cannot be converted to
    OEM code. (not the case at Unicode API cfg.)

When the directory item has no LFN information, lower case characters
can be contained in the `fname[]`.

#### QuickInfo

Available when `_FS_MINIMIZE <= 1`.

#### Sample Code

    FRESULT scan_files (
        char* path        /* Start node to be scanned (also used as work area) */
    )
    {
        FRESULT res;
        FILINFO fno;
        DIR dir;
        int i;
        char *fn;   /* This function is assuming non-Unicode cfg. */
    #if _USE_LFN
        static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
        fno.lfname = lfn;
        fno.lfsize = sizeof lfn;
    #endif
    
    
        res = f_opendir(&dir, path);                       /* Open the directory */
        if (res == FR_OK) {
            i = strlen(path);
            for (;;) {
                res = f_readdir(&dir, &fno);                   /* Read a directory item */
                if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
                if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
    #if _USE_LFN
                fn = *fno.lfname ? fno.lfname : fno.fname;
    #else
                fn = fno.fname;
    #endif
                if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                    sprintf(&path[i], "/%s", fn);
                    res = scan_files(path);
                    if (res != FR_OK) break;
                    path[i] = 0;
                } else {                                       /* It is a file. */
                    printf("%s/%s\n", path, fn);
                }
            }
            f_closedir(&dir)
        }
    
        return res;
    }

#### See Also

`f_opendir, f_closedir, f_stat, FILINFO, DIR`
