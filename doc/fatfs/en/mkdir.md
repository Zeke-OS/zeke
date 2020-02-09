## f\_mkdir

The f\_mkdir function creates a new directory.

    FRESULT f_mkdir (
      const TCHAR* path /* [IN] Directory name */
    );

#### Parameter

  - path  
    Pointer to the null-terminated string that specifies the [directory
    name](filename.md) to create.

#### Return Value

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_PATH](rc.md#np), [FR\_INVALID\_NAME](rc.md#in),
[FR\_DENIED](rc.md#de), [FR\_EXIST](rc.md#ex),
[FR\_WRITE\_PROTECTED](rc.md#wp), [FR\_INVALID\_DRIVE](rc.md#id),
[FR\_NOT\_ENABLED](rc.md#ne), [FR\_NO\_FILESYSTEM](rc.md#ns),
[FR\_TIMEOUT](rc.md#tm), [FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

This function creates a new directory.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_FS_MINIMIZE == 0`.

#### Example

``` 
    res = f_mkdir("sub1");
    if (res) die(res);
    res = f_mkdir("sub1/sub2");
    if (res) die(res);
    res = f_mkdir("sub1/sub2/sub3");
    if (res) die(res);
```
