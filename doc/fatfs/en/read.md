## f\_read

The f\_read function reads data from a file.

```c
FRESULT f_read (
    FIL* fp,     /* [IN] File object */
    void* buff,  /* [OUT] Buffer to store read data */
    UINT btr,    /* [IN] Number of bytes to read */
    UINT* br     /* [OUT] Number of bytes read */
);
```

#### Parameters

  - fp  
    Pointer to the open file object.
  - buff  
    Pointer to the buffer to store read data.
  - btr  
    Number of bytes to read in range of `UINT` type.
  - br  
    Pointer to the `UINT` variable to return number of bytes read. The
    value is always valid after the function call regardless of the
    result.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The file read/write pointer of the file object advances number of bytes
read. After the function succeeded, `*br` should be checked to detect
end of the file. In case of `*br` is less than `btr`, it means the
read/write pointer reached end of the file during read operation.

#### QuickInfo

Always available.

#### See Also

`f_open, fgets, f_write, f_close, FIL`
