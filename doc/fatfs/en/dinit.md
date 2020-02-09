## disk\_initialize

The disk\_initialize function initializes the storage device.

    DSTATUS disk_initialize (
      BYTE pdrv           /* [IN] Physical drive number */
    );

#### Parameter

  - pdrv  
    Physical drive number to identify the target device.

#### Return Values

This function returns a disk status as the result. For details of the
disk status, refer to the [disk\_status()](dstat.md) function.

#### Description

This function initializes a storage device and put it ready to generic
read/write data. When the function succeeded, `STA_NOINIT` flag in the
return value is cleared.

*Application program MUST NOT call this function, or FAT structure on
the volume can be broken. To re-initialize the file system, use
`f_mount()` function instead.* This function is called at volume mount
process by FatFs module to manage the media change.
