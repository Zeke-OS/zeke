## disk\_read

The disk\_read function reads sector(s) from the storage device.

    DRESULT disk_read (
      BYTE pdrv,     /* [IN] Physical drive number */
      BYTE* buff,    /* [OUT] Pointer to the read data buffer */
      DWORD sector,  /* [IN] Start sector number */
      UINT count     /* [IN] Number of sectros to read */
    );

#### Parameters

  - pdrv  
    Physical drive number to identify the target device.
  - buff  
    Pointer to the *byte array* to store the read data.
  - sector  
    Start sector number in logical block address (LBA).
  - count  
    Number of sectors to read. FatFs specifis it in range of from 1 to
    128.

#### Return Value

  - RES\_OK (0)  
    The function succeeded.
  - RES\_ERROR  
    Any hard error occured during the read operation and could not
    recover it.
  - RES\_PARERR  
    Invalid parameter.
  - RES\_NOTRDY  
    The device has not been initialized.

#### Description

The memory address specified by `buff` is not that always aligned to
word boundary because the type of argument is defined as `BYTE*`. The
misaligned read/write request can occure at [direct
transfer](appnote.md#fs1). If the bus architecture, especially DMA
controller, does not allow misaligned memory access, it should be solved
in this function. There are some workarounds described below to avoid
this issue.

  - Convert word transfer to byte transfer in this function. -
    Recommended.
  - For `f_read()`, avoid long read request that includes a whole of
    sector. - Direct transfer will never occure.
  - For `f_read(fp, buff, btr, &br)`, make sure that `(((UINT)buff & 3)
    == (f_tell(fp) & 3))` is true. - Word aligned direct transfer is
    guaranteed.

Generally, a multiple sector transfer request must not be split into
single sector transactions to the storage device, or you will not get
good read throughput.
