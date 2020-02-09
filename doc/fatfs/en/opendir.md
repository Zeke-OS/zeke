## f\_opendir

The f\_opendir function opens a directory.

    FRESULT f_opendir (
      DIR* dp,           /* [OUT] Pointer to the directory object structure */
      const TCHAR* path  /* [IN] Directory name */
    );

#### Parameters

  - dp  
    Pointer to the blank directory object to create a new one.
  - path  
    Pinter to the null-terminated string that specifies the [directory
    name](filename.md) to be opened.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_PATH](rc.md#np), [FR\_INVALID\_NAME](rc.md#in),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_INVALID\_DRIVE](rc.md#id),
[FR\_NOT\_ENABLED](rc.md#ne), [FR\_NO\_FILESYSTEM](rc.md#ns),
[FR\_TIMEOUT](rc.md#tm), [FR\_NOT\_ENOUGH\_CORE](rc.md#nc),
[FR\_TOO\_MANY\_OPEN\_FILES](rc.md#tf)

#### Description

The `f_opendir()` function opens an exsisting directory and creates a
directory object for subsequent `f_readdir()` function.

#### QuickInfo

Available when `_FS_MINIMIZE <= 1`.

#### See Also

`f_readdir, f_closedir, DIR`
