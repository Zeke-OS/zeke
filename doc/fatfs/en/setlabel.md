## f\_setlabel

The f\_setlabel function sets/removes the label of a volume.

    FRESULT f_setlabel (
      const TCHAR* label  /* [IN] Volume label to be set */
    );

#### Parameters

  - label  
    Pointer to the null-terminated string that specifies the volume
    label to be set.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_NAME](rc.md#in), [FR\_WRITE\_PROTECTED](rc.md#wp),
[FR\_INVALID\_DRIVE](rc.md#id), [FR\_NOT\_ENABLED](rc.md#ne),
[FR\_NO\_FILESYSTEM](rc.md#ns), [FR\_TIMEOUT](rc.md#tm)

#### Description

When the string has a drive number, the volume label will be set to the
volume specified by the drive number. If not, the label will be set to
the default drive. If the given string is a null-string, the volume
label on the volume will be removed. The format of the volume label is
similar to the short file name but there are some differences shown
below:

  - 11 bytes or less in length as local character code. LFN extention is
    not applied to the volume label.
  - Cannot contain period.
  - Can contain spaces anywhere in the volume label. Trailing spaces are
    truncated off.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_USE_LABEL == 1`.

#### Example

``` 
    /* Set volume label to the default drive */
    f_setlabel("DATA DISK");

    /* Set volume label to the drive 2 */
    f_setlabel("2:DISK 3 OF 4");

    /* Remove volume label of the drive 2 */
    f_setlabel("2:");
```

#### See Also

`f_getlabel`
