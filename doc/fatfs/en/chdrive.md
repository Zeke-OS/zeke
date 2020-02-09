## f\_chdrive

The f\_chdrive function changes the current drive.

    FRESULT f_chdrive (
      const TCHAR* path  /* [IN] Logical drive number */
    );

#### Parameters

  - path  
    Specifies the [logical drive number](filename.md) to be set as the
    current drive.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_INVALID\_DRIVE](rc.md#id)

#### Description

The `f_chdrive()` function changes the current drive. The initial value
of the current drive number is 0. Note that the current drive is
retained in a static variable so that it also affects other tasks that
using the file functions.

#### QuickInfo

Available when `_FS_RPATH >= 1` and `_VOLUMES >= 2`.

#### See Also

`f_chdir, f_getcwd`
