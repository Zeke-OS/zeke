## f\_mount

The f\_mount fucntion registers/unregisters file system object to the
FatFs module.

    FRESULT f_mount (
      FATFS*       fs,    /* [IN] File system object */
      const TCHAR* path,  /* [IN] Logical drive number */
      BYTE         opt    /* [IN] Initialization option */
    );

#### Parameters

  - fs  
    Pointer to the new file system object to be registered. Null pointer
    unregisters the registered file system object.
  - path  
    Pointer to the null-terminated string that specifies the [logical
    drive](filename.md). The string with no drive number means the
    default drive.
  - opt  
    Initialization option. 0: Do not mount now (to be mounted later), 1:
    Force mounted the volume to check if the FAT volume is available.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_INVALID\_DRIVE](rc.md#id),
[FR\_DISK\_ERR](rc.md#de), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_FILESYSTEM](rc.md#ns)

#### Description

The `f_mount()` function registers/unregisters a file system object used
for the logical drive to the FatFs module as follows:

1.  Determines the logical drive which specified by `path`.
2.  Clears and unregisters the regsitered work area of the drive.
3.  Clears and registers the new work area to the drive if `fs` is not
    NULL.
4.  Performs volume mount process to the drive if forced mount is
    specified.

The file system object is the work area needed for each logical drive.
It must be given to the logical drive with this function prior to use
any other file functions except for `f_fdisk()` function. To unregister
a work area, specify a NULL to the `fs`, and then the work area can be
discarded.

If forced mount is not specified, this function always succeeds
regardless of the physical drive status due to delayed mount feature. It
only clears (de-initializes) the given work area and registers its
address to the internal table. No activity of the physical drive in this
function. It can also be used to force de-initialized the registered
work area of a logical drive. The volume mount processes, initialize the
corresponding physical drive, find the FAT volume in it and initialize
the work area, is performed in the subsequent file access functions when
either or both of following condition is true.

  - File system object is not initialized. It is cleared by `f_mount()`.
  - Physical drive is not initialized. It is de-initialized by system
    reset or media removal.

If the function with forced mount failed, it means that the file system
object has been registered successfully but the volume is currently not
ready to use. Mount process will also be attempted in subsequent file
access functions.

If implementation of the disk I/O layer lacks media change detection,
application program needs to perform a `f_mount()` after each media
change to force cleared the file system object.

#### QuickInfo

Always available.

#### See Also

`f_open`, `FATFS`
