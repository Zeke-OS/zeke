## f\_close

The f\_close function closes an open file.

    FRESULT f_close (
      FIL* fp     /* [IN] Pointer to the file object */
    );

#### Parameter

  - fp  
    Pointer to the open file object structure to be closed.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The `f_close()` function closes an open file object. If any data has
been written to the file, the cached information of the file is written
back to the volume. After the function succeeded, the file object is no
longer valid and it can be discarded.

Note that if the file object is in read-only mode and `_FS_LOCK` option
is not enabled, the file object can also be discarded without this
process. However this is not recommended for future compatibility.

#### QuickInfo

Always available.

#### See Also

`f_open, f_read, f_write, f_sync, FIL, FATFS`
