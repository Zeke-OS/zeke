## f\_stat

The f\_stat function checks the existence of a file or sub-directory.

    FRESULT f_stat (
      const TCHAR* path,  /* [IN] Object name */
      FILINFO* fno        /* [OUT] FILINFO structure */
    );

#### Parameters

  - path  
    Pointer to the null-terminated string that specifies the
    [object](filename.md) to get its information.
  - fno  
    Pointer to the blank `FILINFO` structure to store the information of
    the object. Set null pointer if it is not needed.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_FILE](rc.md#ok), [FR\_NO\_PATH](rc.md#np),
[FR\_INVALID\_NAME](rc.md#in), [FR\_INVALID\_DRIVE](rc.md#id),
[FR\_NOT\_ENABLED](rc.md#ne), [FR\_NO\_FILESYSTEM](rc.md#ns),
[FR\_TIMEOUT](rc.md#tm), [FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

The `f_stat()` function checks the existence of a file or sub-directory.
If not exist, the function returns with `FR_NO_FILE`. If exist, the
function returns with `FR_OK` and the informations of the object, file
size, timestamp, attribute and SFN, are stored to the file information
structure. For details of the file information, refer to the `FILINFO`
structure and [`f_readdir()`](readdir.md) function.

#### QuickInfo

Available when `_FS_MINIMIZE == 0`.

#### References

`f_opendir, f_readdir, FILINFO`
