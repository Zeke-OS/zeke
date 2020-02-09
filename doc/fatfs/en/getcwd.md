## f\_getcwd

The f\_getcwd function retrieves the current directory.

    FRESULT f_getcwd (
      TCHAR* buff, /* [OUT] Buffer to return path name */
      UINT len     /* [IN] The length of the buffer */
    );

#### Parameters

  - buff  
    Pointer to the buffer to receive the current directory string.
  - len  
    Size of the buffer in unit of TCHAR.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NOT\_ENABLED](rc.md#ne), [FR\_NO\_FILESYSTEM](rc.md#ns),
[FR\_TIMEOUT](rc.md#tm), [FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

The `f_getcwd()` function retrieves full path name of the current
directory of the current drive. When `_VOLUMES` is larger than 1, a
logical drive number is added to top of the path name.

#### QuickInfo

Available when `_FS_RPATH == 2`.

#### See Also

`f_chdrive, f_chdir`
