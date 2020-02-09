## f\_getlabel

The f\_getlabel function returns volume label and volume serial number
of a drive.

    FRESULT f_getlabel (
      const TCHAR* path,  /* [IN] Drive number */
      TCHAR* label,       /* [OUT] Volume label */
      DWORD* vsn          /* [OUT] Volume serial number */
    );

#### Parameters

  - path  
    Pointer to the null-terminated string that specifies the [logical
    drive](filename.md). Null-string specifies the default drive.
  - label  
    Pointer to the buffer to store the volume label. The buffer size
    must be at least 12 items. If the volume has no label, a null-string
    will be returned. Set null pointer if this information is not
    needed.
  - vsn  
    Pointer to the `DWORD` variable to store the volume serial number.
    Set null pointer if this information is not needed.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm)

#### QuickInfo

Available when `_USE_LABEL == 1`.

#### Example

``` 
    char str[12];

    /* Get volume label of the default drive */
    f_getlabel("", str, 0);

    /* Get volume label of the drive 2 */
    f_getlabel("2:", str, 0);
```

#### See Also

`f_setlabel`
