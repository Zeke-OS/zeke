## f\_rename

Renames a file or sub-directory.

    FRESULT f_rename (
      const TCHAR* old_name, /* [IN] Old object name */
      const TCHAR* new_name  /* [IN] New object name */
    );

#### Parameters

  - old\_name  
    Pointer to a null-terminated string that specifies an existing [file
    or sub-directory](filename.md) to be renamed.
  - new\_name  
    Pointer to a null-terminated string that specifies the new object
    name. The drive number specified in this string is ignored and one
    determined by `old_name` is used instead.

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

Renames a file or sub-directory and can also move it to other directory
within the same logical drive. *Do not rename open objects* or directry
table can be broken.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_FS_MINIMIZE == 0`.

#### Example

``` 
    /* Rename an object */
    f_rename("oldname.txt", "newname.txt");

    /* Rename and move an object to other directory */
    f_rename("oldname.txt", "dir1/newname.txt");
```
