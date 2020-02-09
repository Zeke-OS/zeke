## disk\_write

The disk\_write writes sector(s) to the storage device.

    DRESULT disk_write (
      BYTE drv,         /* [IN] Physical drive number */
      const BYTE* buff, /* [IN] Pointer to the data to be written */
      DWORD sector,     /* [IN] Sector number to write from */
      UINT count        /* [IN] Number of sectors to write */
    );

#### Parameters

  - pdrv  
    Physical drive number to identify the target device.
  - buff  
    Pointer to the *byte array* to be written. The size of data to be
    written is sector size \* `count` bytes.
  - sector  
    Start sector number in logical block address (LBA).
  - count  
    Number of sectors to write. FatFs specifis it in range of from 1 to
    128.

#### Return Values

  - RES\_OK (0)  
    The function succeeded.
  - RES\_ERROR  
    Any hard error occured during the write operation and could not
    recover it.
  - RES\_WRPRT  
    The medium is write protected.
  - RES\_PARERR  
    Invalid parameter.
  - RES\_NOTRDY  
    The device has not been initialized.

#### Description

The specified memory address is not that always aligned to word boundary
because the type of pointer is defined as `BYTE*`. For more information,
refer to the description of [`disk_read()`](dread.md) function.

Generally, a multiple sector transfer request must not be split into
single sector transactions to the storage device, or you will never get
good write throughput.

FatFs expects delayed write feature of the disk functions. The write
operation to the media need not to be completed due to write operation
is in progress or only stored it into the cache buffer when return from
this function. But data on the `buff` is invalid after return from this
function. The write completion request is done by `CTRL_SYNC` command of
`disk_ioctl()` function. Therefore, if delayed write feature is
implemented, the write throughput may be improved.

*Application program MUST NOT call this function, or FAT structure on
the volume can be collapsed.*

#### QuickInfo

This function is not needed when `_FS_READONLY == 1`.
