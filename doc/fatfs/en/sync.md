## f\_sync

The f\_sync function flushes the cached information of a writing file.

```c
FRESULT f_sync (
    FIL* fp     /* [IN] File object */
);
```

#### Parameter

  - fp  
    Pointer to the open file object to be flushed.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The `f_sync()` function performs the same process as `f_close()`
function but the file is left opened and can continue read/write/seek
operations to the file. This is suitable for the applications that open
files for a long time in write mode, such as data logger. Performing
`f_sync()` function of periodic or immediataly after `f_write()`
function can minimize the risk of data loss due to a sudden blackout or
an unintentional media removal. For more information, refer to
[application note](appnote.md#critical).

However there is no sense in `f_sync()` function immediataly before
`f_close()` function because it performs `f_sync()` function in it. In
other words, the differnce between those functions is that the file
object is invalidated or not.

#### QuickInfo

Available when `_FS_READONLY == 0`.

#### See Also

`f_close`
