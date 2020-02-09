## f\_truncate

The f\_truncate function truncates the file size.

    FRESULT f_truncate (
      FIL* fp     /* [IN] File object */
    );

#### Parameter

  - fp  
    Pointer to the open file object to be truncated.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The `f_truncate()` function truncates the file size to the current file
read/write pointer. This function has no effect if the file read/write
pointer is already pointing end of the file.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_FS_MINIMIZE == 0`.

#### See Also

`f_open, f_lseek, FIL`
