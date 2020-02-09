## f\_utime

The f\_utime function changes the timestamp of a file or sub-directory.

```c
FRESULT f_utime (
    const TCHAR* path,  /* [IN] Object name */
    const FILINFO* fno  /* [IN] Time and data to be set */
);
```

#### Parameters

  - path  
    Pointer to the null-terminated string that specifies an
    [object](filename.md) to be changed.
  - fno  
    Pointer to the file information structure that has a timestamp to be
    set in member fdate and ftime. Do not care any other members.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_FILE](rc.md#ok), [FR\_NO\_PATH](rc.md#np),
[FR\_INVALID\_NAME](rc.md#in), [FR\_WRITE\_PROTECTED](rc.md#wp),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm),
[FR\_NOT\_ENOUGH\_CORE](rc.md#nc)

#### Description

The `f_utime()` function changes the timestamp of a file or
sub-directory

#### Example

    FRESULT set_timestamp (
        char *obj,     /* Pointer to the file name */
        int year,
        int month,
        int mday,
        int hour,
        int min,
        int sec
    )
    {
        FILINFO fno;
    
        fno.fdate = (WORD)(((year - 1980) * 512U) | month * 32U | mday);
        fno.ftime = (WORD)(hour * 2048U | min * 32U | sec / 2U);
    
        return f_utime(obj, &fno);
    }

#### QuickInfo

Available when `_FS_READONLY == 0`.

#### See Also

`f_stat, FILINFO`
