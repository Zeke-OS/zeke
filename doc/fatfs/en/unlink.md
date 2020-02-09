## f\_unlink

The f\_unlink function removes a file or sub-directory.

```c
FRESULT f_unlink (
    const TCHAR* path  /* [IN] Object name */
);
```

#### Parameter

  - path  
    Pointer to the null-terminated string that specifies an
    [object](filename.md) to be removed.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_FILE](rc.md#ok), [FR\_NO\_PATH](rc.md#np),
[FR\_INVALID\_NAME](rc.md#in), [FR\_DENIED](rc.md#de),
[FR\_EXIST](rc.md#ex), [FR\_WRITE\_PROTECTED](rc.md#wp),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm),
[FR\_LOCKED](rc.md#lo), [FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

If condition of the object to be removed is applicable to the following
terms, the function will be rejected.

  - The file/sub-directory must not have read-only attribute (`AM_RDO`),
    or the function will be rejected with `FR_DENIED`.
  - The sub-directory must be empty and must not be current directory,
    or the function will be rejected with `FR_DENIED`.
  - The file/sub-directory must not be opened, or the *FAT volume can be
    collapsed*. It can be rejected with `FR_LOCKED` when [file lock
    feature](appnote.md#dup) is enabled.
