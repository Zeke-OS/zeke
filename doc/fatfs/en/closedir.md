## f\_closedir

The f\_closedir function closes an open directory.

    FRESULT f_closedir (
      DIR* dp     /* [IN] Pointer to the directory object */
    );

#### Parameter

  - dp  
    Pointer to the open directory object structure to be closed.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_INT\_ERR](rc.md#ie),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The `f_closedir()` function closes an open directory object. After the
function succeeded, the directory object is no longer valid and it can
be discarded.

Note that the directory object can also be discarded without this
process if `_FS_LOCK` option is not enabled. However this is not
recommended for future compatibility.

#### QuickInfo

Available when `_FS_MINIMIZE <= 1`.

#### See Also

`f_opendir, f_readdir, DIR`
