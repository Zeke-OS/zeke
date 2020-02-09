## disk\_ioctl

The disk\_ioctl function cntrols device specific features and
miscellaneous functions other than generic read/write.

    DRESULT disk_ioctl (
      BYTE pdrv,     /* [IN] Drive number */
      BYTE cmd,      /* [IN] Control command code */
      void* buff     /* [I/O] Parameter and data buffer */
    );

#### Parameters

  - pdrv  
    Physical drive number to identify the target device.
  - cmd  
    Command code.
  - buff  
    Pointer to the parameter depends on the command code. Do not care if
    no parameter to be passed.

#### Return Value

  - RES\_OK (0)  
    The function succeeded.
  - RES\_ERROR  
    An error occured.
  - RES\_PARERR  
    The command code or parameter is invalid.
  - RES\_NOTRDY  
    The device has not been initialized.

#### Description

The FatFs module requires only five device independent commands
described
below.

| Command             | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| ------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| CTRL\_SYNC          | Make sure that the device has finished pending write process. If the disk I/O module has a write back cache, the dirty buffers must be written back to the media immediately. Nothing to do for this command if each write operation to the media is completed within the `disk_write()` function. Required at `_FS_READONLY == 0`.                                                                                                                                                                                                                                                                                        |
| GET\_SECTOR\_COUNT  | Returns number of available sectors on the drive into the `DWORD` variable pointed by `buff`. This command is used by only `f_mkfs()` and `f_fdisk()` function to determine the volume/partition size to be created. Required at `_USE_MKFS == 1` or `_MULTI_PARTITION == 1`.                                                                                                                                                                                                                                                                                                                                              |
| GET\_SECTOR\_SIZE   | Returns sector size of the media into the `WORD` variable pointed by `buff`. Valid return values of this command are 512, 1024, 2048 or 4096. This command is required at variable sector size configuration, `_MAX_SS > _MIN_SS`. Never used at fixed sector size configuration, `_MAX_SS == _MIN_SS`, and it must work at that sector size.                                                                                                                                                                                                                                                                              |
| GET\_BLOCK\_SIZE    | Returns erase block size of the flash memory in unit of sector into the `DWORD` variable pointed by `buff`. The allowable value is from 1 to 32768 in power of 2. Return 1 if the erase block size is unknown or disk media. This command is used by only `f_mkfs()` function and it attempts to align data area to the erase block boundary. Required at `_USE_MKFS == 1`.                                                                                                                                                                                                                                                |
| CTRL\_ERASE\_SECTOR | Informs device that the data on the block of sectors specified by a `DWORD` array {\<start sector\>, \<end sector\>} pointed by `buff` is no longer needed and may be erased. The device would force erased the memory block. This is a command similar to Trim command of ATA device. When this feature is not supported or not a flash memory media, nothing to do for this command. The FatFs does not check the result code and the file function is not affected even if the sectors ware not erased well. This command is called on removing a cluster chain and `f_mkfs()` function. Required at `_USE_ERASE == 1`. |

Standard ioctl command used by FatFs

FatFs never uses any device dependent command and user defined command.
Following table shows an example of non-standard commands usable for
some
applications.

| Command           | Description                                                                                                                                                                                                         |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| CTRL\_FORMAT      | Create a physical format on the media. If `buff` is not null, it is pointer to the call-back function for progress notification.                                                                                    |
| CTRL\_POWER\_IDLE | Put the device idle state. `STA_NOINIT` in status flag may not be set if the device would go active state by generic read/write function.                                                                           |
| CTRL\_POWER\_OFF  | Put the device off state. Shut-down the power to the device and deinitialize the device interface if needed. `STA_NOINIT` in status flag must be set. The device goes active state by `disk_initialize()` function. |
| CTRL\_LOCK        | Lock media eject mechanism.                                                                                                                                                                                         |
| CTRL\_UNLOCK      | Unlock media eject mechanism.                                                                                                                                                                                       |
| CTRL\_EJECT       | Eject media cartridge. `STA_NOINIT` and `STA_NODISK` in status flag are set after the function succeeded.                                                                                                           |
| MMC\_GET\_TYPE    | Get card type. The type flags, bit0:MMCv3, bit1:SDv1, bit2:SDv2+ and bit3:LBA, is stored to a BYTE variable pointed by `buff`. (MMC/SDC specific command)                                                           |
| MMC\_GET\_CSD     | Get CSD register into a 16-byte buffer pointed by `buff`. (MMC/SDC specific command)                                                                                                                                |
| MMC\_GET\_CID     | Get CID register into a 16-byte buffer pointed by `buff`. (MMC/SDC specific command)                                                                                                                                |
| MMC\_GET\_OCR     | Get OCR register into a 4-byte buffer pointed by `buff`. (MMC/SDC specific command)                                                                                                                                 |
| MMC\_GET\_SDSTAT  | Get SDSTATUS register into a 64-byte buffer pointed by `buff`. (SDC specific command)                                                                                                                               |
| ATA\_GET\_REV     | Get the revision string into a 16-byte buffer pointed by `buff`. (ATA/CFC specific command)                                                                                                                         |
| ATA\_GET\_MODEL   | Get the model string into a 40-byte buffer pointed by `buff`. (ATA/CFC specific command)                                                                                                                            |
| ATA\_GET\_SN      | Get the serial number string into a 20-byte buffer pointed by `buff`. (ATA/CFC specific command)                                                                                                                    |

Example of optional ioctl command
