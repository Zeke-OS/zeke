## f\_chmod

The f\_chmod function changes the attribute of a file or sub-directory.

    FRESULT f_chmod (
      const TCHAR* path, /* [IN] Object name */
      BYTE attr,         /* [IN] Attribute flags */
      BYTE mask          /* [IN] Attribute masks */
    );

#### Parameters

  - path  
    Pointer to the null-terminated string that specifies an
    [object](filename.md) to be changed
  - attr  
    Attribute flags to be set in one or more combination of the
    following flags. The specified flags are set and others are
    cleard.  
    | Attribute | Description |
    | --------- | ----------- |
    | AM\_RDO   | Read only   |
    | AM\_ARC   | Archive     |
    | AM\_SYS   | System      |
    | AM\_HID   | Hidden      |
  - mask  
    Attribute mask that specifies which attribute is changed. The
    specified attributes are set or cleard and others are left
    unchanged.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_FILE](rc.md#ok), [FR\_NO\_PATH](rc.md#np),
[FR\_INVALID\_NAME](rc.md#in), [FR\_WRITE\_PROTECTED](rc.md#wp),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm),
[FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

The `f_chmod()` function changes the attribute of a file or
sub-directory.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_FS_MINIMIZE
== 0`.

#### Example

``` 
    /* Set read-only flag, clear archive flag and others are left unchanged. */
    f_chmod("file.txt", AR_RDO, AR_RDO | AR_ARC);
```
