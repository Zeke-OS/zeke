# Return Code of the File Functions

On the FatFs API, most of file functions return common result code as
enum type `FRESULT`. When a function succeeded, it returns zero,
otherwise returns non-zero value that indicates type of error.

  - FR\_OK (0)  
    The function succeeded.
  - FR\_DISK\_ERR  
    An unrecoverable hard error occured in the lower layer,
    `disk_read()`, `disk_write()` or `disk_ioctl()` function.  
    Note that if once this error occured at any operation to an open
    file, the file object is aborted and all operations to the file
    except for close will be rejected.
  - FR\_INT\_ERR  
    Assertion failed. An insanity is detected in the internal process.
    One of the following possibilities is suspected.
      - Work area (file system object, file object or etc...) is broken
        by stack overflow or any other application. This is the reason
        in most case.
      - There is any error of the FAT structure on the volume.
    Note that if once this error occured at any operation to an open
    file, the file object is aborted and all operations to the file
    except for close will be rejected.
  - FR\_NOT\_READY  
    The disk drive cannot work due to incorrect medium removal or
    `disk_initialize()` function failed.
  - FR\_NO\_FILE  
    Could not find the file.
  - FR\_NO\_PATH  
    Could not find the path.
  - FR\_INVALID\_NAME  
    The given string is invalid as the [path name](filename.md).
  - FR\_DENIED  
    The required access was denied due to one of the following reasons:
      - Write mode open against the read-only file.
      - Deleting the read-only file or directory.
      - Deleting the non-empty directory or current directory.
      - Reading the file opened without `FA_READ` flag.
      - Any modification to the file opened without `FA_WRITE` flag.
      - Could not create the file or directory due to the directory
        table is full.
      - Could not create the directory due to the volume is full.
  - FR\_EXIST  
    Name collision. Any object that has the same name is already
    existing.
  - FR\_INVALID\_OBJECT  
    The file/directory object structure is invalid or a null pointer is
    given. All open objects of the logical drive are invalidated by the
    voulme mount process.
  - FR\_WRITE\_PROTECTED  
    Any write mode action against the write-protected media.
  - FR\_INVALID\_DRIVE  
    Invalid drive number is specified in the path name. A null pointer
    is given as the path name. (Related option: `_VOLUMES`)
  - FR\_NOT\_ENABLED  
    Work area for the logical drive has not been registered by
    `f_mount()` function.
  - FR\_NO\_FILESYSTEM  
    There is no valid FAT volume on the drive.
  - FR\_MKFS\_ABORTED  
    The `f_mkfs()` function aborted before start in format due to a
    reason as follows:
      - The disk/partition size is too small.
      - Not allowable cluster size for this disk. This can occure when
        number of clusters gets near the boundaries of FAT sub-types.
      - There is no partition related to the logical drive. (Related
        option: `_MULTI_PARTITION`)
  - FR\_TIMEOUT  
    The function was canceled due to a timeout of [thread-safe
    control](appnote.md#reentrant). (Related option: `_TIMEOUT`)
  - FR\_LOCKED  
    The operation to the object was rejected by [file sharing
    control](appnote.md#dup). (Related option: `_FS_LOCK`)
  - FR\_NOT\_ENOUGH\_CORE  
    Not enough memory for the operation. There is one of the following
    reasons:
      - Could not allocate a memory for LFN working buffer. (Related
        option: `_USE_LFN`)
      - Size of the given CLMT buffer is insufficient for the file
        fragments.
  - FR\_TOO\_MANY\_OPEN\_FILES  
    Number of open objects has been reached maximum value and no more
    object can be opened. (Related option: `_FS_LOCK`)
  - FR\_INVALID\_PARAMETER  
    The given parameter is invalid or there is any inconsistent.
