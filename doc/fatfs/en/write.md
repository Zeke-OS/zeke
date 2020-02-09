## f\_write

The f\_write writes data to a file.

```c
FRESULT f_write (
    FIL* fp,          /* [IN] Pointer to the file object structure */
    const void* buff, /* [IN] Pointer to the data to be written */
    UINT btw,         /* [IN] Number of bytes to write */
    UINT* bw          /* [OUT] Pointer to the variable to return number of bytes written */
);
```

#### Parameters

  - fp  
    Pointer to the open file object structure.
  - buff  
    Pointer to the data to be written.
  - btw  
    Specifies number of bytes to write in range of `UINT` type.
  - bw  
    Pointer to the `UINT` variable to return the number of bytes
    written. The value is always valid after the function call
    regardless of the result.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The read/write pointer of the file object advances number of bytes
written. After the function succeeded, `*bw` should be checked to detect
the disk full. In case of `*bw` is less than `btw`, it means the volume
got full during the write operation. The function can take a time when
the volume is full or close to full.

#### QuickInfo

Available when `_FS_READONLY == 0`.

#### See Also

`f_open, f_read, fputc, fputs, fprintf, f_close, FIL`
