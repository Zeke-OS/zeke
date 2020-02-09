## f\_lseek

The f\_lseek function moves the file read/write pointer of an open file
object. It can also be used to expand the file size (cluster
pre-allocation).

    FRESULT f_lseek (
      FIL* fp,   /* [IN] File object */
      DWORD ofs  /* [IN] File read/write pointer */
    );

#### Parameters

  - fp  
    Pointer to the open file object.
  - ofs  
    Byte offset from top of the file.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The `f_lseek()` function moves the file read/write pointer of an open
file. The offset can be specified in only origin from top of the file.
When an offset beyond the file size is specified at write mode, the file
size is expanded to the specified offset. The file data in the expanded
area is undefined because no data is written to the file. This is
suitable to pre-allocate a cluster chain quickly, for fast write
operation. After the `f_lseek()` function succeeded, the current
read/write pointer should be checked in order to make sure the
read/write pointer has been moved correctry. In case of the current
read/write pointer is not the expected value, either of followings has
been occured.

  - End of file. The specified `ofs` was clipped at end of the file
    because the file has been opened in read-only mode.
  - Disk full. There is insufficient free space on the volume to expand
    the file.

Fast seek feature is enabled when `_USE_FASTSEEK` is set to 1 and the
member `cltbl` in the file object is not NULL. This feature enables fast
backward/long seek operations without FAT access by using CLMT (cluster
link map table). The fast seek feature is also applied to
`f_read()/f_write()` function, however, the file size cannot be expanded
by `f_write()/f_lseek()` function.

The CLMT must be created in the user defined `DWORD` array prior to use
the fast seek feature. To create the CLMT, set address of the `DWORD`
array to the member `cltbl` in the file object, set the array size in
unit of items into the first item and call the `f_lseek()` function with
`ofs`` = CREATE_LINKMAP`. After the function succeeded and CLMT is
created, no FAT access is occured at subsequent
`f_read()/f_write()/f_lseek()` function to the file. If the function
failed with `FR_NOT_ENOUGH_CORE`, the given array size is insufficient
for the file and number of items required is returned into the first
item of the array. The required array size is (number of fragments + 1)
\* 2 items. For example, when the file is fragmented in 5, 12 items will
be required for the CLMT.

#### QuickInfo

Available when `_FS_MINIMIZE <= 2`.

#### Example

``` 
    /* Open file */
    fp = malloc(sizeof (FIL));
    res = f_open(fp, "file.dat", FA_READ|FA_WRITE);
    if (res) ...

    /* Move to offset of 5000 from top of the file */
    res = f_lseek(fp, 5000);

    /* Move to end of the file to append data */
    res = f_lseek(fp, f_size(fp));

    /* Forward 3000 bytes */
    res = f_lseek(fp, f_tell(fp) + 3000);

    /* Rewind 2000 bytes (take care on wraparound) */
    res = f_lseek(fp, f_tell(fp) - 2000);
```

    /* Cluster pre-allocation (to prevent buffer overrun on streaming write) */
    
        res = f_open(fp, recfile, FA_CREATE_NEW | FA_WRITE);   /* Create a file */
    
        res = f_lseek(fp, PRE_SIZE);             /* Expand file size (cluster pre-allocation) */
        if (res || f_tell(fp) != PRE_SIZE) ...   /* Check if the file has been expanded */
    
        res = f_lseek(fp, DATA_START);           /* Record data stream WITHOUT cluster allocation delay */
        ...                                      /* DATA_START and write block size should be aligned to sector boundary */
    
        res = f_truncate(fp);                    /* Truncate unused area */
        res = f_lseek(fp, 0);                    /* Put file header */
        ...
    
        res = f_close(fp);

    /* Using fast seek feature */
    
        DWORD clmt[SZ_TBL];                    /* Cluster link map table buffer */
    
        res = f_lseek(fp, ofs1);               /* This is normal seek (cltbl is nulled on file open) */
    
        fp->cltbl = clmt;                      /* Enable fast seek feature (cltbl != NULL) */
        clmt[0] = SZ_TBL;                      /* Set table size */
        res = f_lseek(fp, CREATE_LINKMAP);     /* Create CLMT */
        ...
    
        res = f_lseek(fp, ofs2);               /* This is fast seek */

#### See Also

`f_open, f_truncate, FIL`
