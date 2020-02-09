## f\_forward

The f\_forward function reads the file data and forward it to the data
streaming device.

    FRESULT f_forward (
      FIL* fp,                        /* [IN] File object */
      UINT (*func)(const BYTE*,UINT), /* [IN] Data streaming function */
      UINT btf,                       /* [IN] Number of bytes to forward */
      UINT* bf                        /* [OUT] Number of bytes forwarded */
    );

#### Parameters

  - fp  
    Pointer to the open file object.
  - func  
    Pointer to the user-defined data streaming function. For details,
    refer to the sample code.
  - btf  
    Number of bytes to forward in range of `UINT`.
  - bf  
    Pointer to the `UINT` variable to return number of bytes forwarded.

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_INVALID\_OBJECT](rc.md#io), [FR\_TIMEOUT](rc.md#tm)

#### Description

The `f_forward()` function reads the data from the file and forward it
to the outgoing stream without data buffer. This is suitable for small
memory system because it does not require any data buffer at application
module. The file pointer of the file object increases in number of bytes
forwarded. In case of `*bf` is less than `btf` without error, it means
the requested bytes could not be transferred due to end of file or
stream goes busy during data transfer.

#### QuickInfo

Available when `_USE_FORWARD == 1` and `_FS_TINY
    == 1`.

#### Example (Audio playback)

    /*------------------------------------------------------------------------*/
    /* Sample code of data transfer function to be called back from f_forward */
    /*------------------------------------------------------------------------*/
    
    UINT out_stream (   /* Returns number of bytes sent or stream status */
        const BYTE *p,  /* Pointer to the data block to be sent */
        UINT btf        /* >0: Transfer call (Number of bytes to be sent). 0: Sense call */
    )
    {
        UINT cnt = 0;
    
    
        if (btf == 0) {     /* Sense call */
            /* Return stream status (0: Busy, 1: Ready) */
            /* When once it returned ready to sense call, it must accept a byte at least */
            /* at subsequent transfer call, or f_forward will fail with FR_INT_ERR. */
            if (FIFO_READY) cnt = 1;
        }
        else {              /* Transfer call */
            do {    /* Repeat while there is any data to be sent and the stream is ready */
                FIFO_PORT = *p++;
                cnt++;
            } while (cnt < btf && FIFO_READY);
        }
    
        return cnt;
    }
    
    
    /*------------------------------------------------------------------------*/
    /* Sample code using f_forward function                                   */
    /*------------------------------------------------------------------------*/
    
    FRESULT play_file (
        char *fn        /* Pointer to the audio file name to be played */
    )
    {
        FRESULT rc;
        FIL fil;
        UINT dmy;
    
        /* Open the audio file in read only mode */
        rc = f_open(&fil, fn, FA_READ);
        if (rc) return rc;
    
        /* Repeat until the file pointer reaches end of the file */
        while (rc == FR_OK && fil.fptr < fil.fsize) {
    
            /* any other processes... */
    
            /* Fill output stream periodicaly or on-demand */
            rc = f_forward(&fil, out_stream, 1000, &dmy);
        }
    
        /* Close the file and return */
        f_close(&fil);
        return rc;
    }

#### See Also

`f_open, fgets, f_write, f_close, FIL`
