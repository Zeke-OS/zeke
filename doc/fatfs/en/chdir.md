## f\_chdir

The f\_chdir function changes the current directory of a drive.

    FRESULT f_chdir (
      const TCHAR* path /* [IN] Path name */
    );

#### Parameters

  - path  
    Pointer to the null-terminated string that specifies a
    [directory](filename.md) to go.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_PATH](rc.md#np), [FR\_INVALID\_NAME](rc.md#in),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm),
[FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

The `f_chdir()` function changes the current directory of the logical
drive. The current directory of a drive is initialized to the root
directory when the drive is auto-mounted. Note that the current
directory is retained in the each file system object so that it also
affects other tasks that using the volume.

#### QuickInfo

Available when `_FS_RPATH
>= 1`.

#### Example

``` 
    /* Change current direcoty of the current drive (dir1 under root dir) */
    f_chdir("/dir1");

    /* Change current direcoty of drive 2 (parent dir) */
    f_chdir("2:..");
```

#### See Also

`f_chdrive, f_getcwd`
