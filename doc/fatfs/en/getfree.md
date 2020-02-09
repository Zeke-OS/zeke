## f\_getfree

The f\_getfree function gets number of the free clusters on the volume.

    FRESULT f_getfree (
      const TCHAR* path,  /* [IN] Logical drive number */
      DWORD* nclst,       /* [OUT] Number of free clusters */
      FATFS** fatfs       /* [OUT] Corresponding file system object */
    );

#### Parameters

  - path  
    Pinter to the null-terminated string that specifies the [logical
    drive](filename.md). A null-string means the default drive.
  - nclst  
    Pointer to the `DWORD` variable to store number of free clusters.
  - fatfs  
    Pointer to pointer that to store a pointer to the corresponding file
    system object.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm)

#### Descriptions

The `f_getfree()` function gets number of free clusters on the volume.
The member `csize` in the file system object indicates number of sectors
per cluster, so that the free space in unit of sector can be calcurated
with this information. When FSINFO structure on the FAT32 volume is not
in sync, this function can return an incorrect free cluster count. To
avoid this problem, FatFs can be forced full FAT scan by `_FS_NOFSINFO`
option.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_FS_MINIMIZE == 0`.

#### Example

``` 
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;


    /* Get volume information and free clusters of drive 1 */
    res = f_getfree("1:", &fre_clust, &fs);
    if (res) die(res);

    /* Get total sectors and free sectors */
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    /* Print the free space (assuming 512 bytes/sector) */
    printf("%10lu KiB total drive space.\n%10lu KiB available.\n",
           tot_sect / 2, fre_sect / 2);
```

#### See Also

`FATFS`
