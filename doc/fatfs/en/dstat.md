## disk\_status

The disk\_status function returns the current disk status.

    DSTATUS disk_status (
      BYTE pdrv     /* [IN] Physical drive number */
    );

#### Parameter

  - pdrv  
    Physical drive number to identify the target device.

#### Return Values

The disk status is returned in combination of following flags. FatFs
refers only `STA_NOINIT` and `STA_PROTECT`.

  - STA\_NOINIT  
    Indicates that the device is not initialized. This flag is set on
    system reset, media removal or failure of `disk_initialize()`
    function. It is cleared on `disk_initialize()` function succeeded.
    Media change that occurs asynchronously must be captured and reflect
    it to the status flags, or auto-mount feature will not work
    correctly. When media change detection feature is not supported,
    application program needs to de-initialize the file system object
    with `f_mount()` function after the media change.
  - STA\_NODISK  
    Indicates that no medium in the drive. This is always cleared on
    fixed disk drive. Note that FatFs does not refer this flag.
  - STA\_PROTECT  
    Indicates that the medium is write protected. This is always cleared
    on the drives without write protect feature. Not valid while no
    medium in the drive.
