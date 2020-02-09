## f\_open

The f\_open function creates a *file object* to be used to access the
file.

    FRESULT f_open (
      FIL* fp,           /* [OUT] Pointer to the file object structure */
      const TCHAR* path, /* [IN] File name */
      BYTE mode          /* [IN] Mode flags */
    );

#### Parameters

  - fp  
    Pointer to the blank file object structure to be created.
  - path  
    Pointer to a null-terminated string that specifies the [file
    name](filename.md) to open or create.
  - mode  
    Mode flags that specifies the type of access and open method for the
    file. It is specified by a combination of following flags.  
    <table>
    <thead>
    <tr class="header">
    <th>Value</th>
    <th>Description</th>
    </tr>
    </thead>
    <tbody>
    <tr class="odd">
    <td>FA_READ</td>
    <td>Specifies read access to the object. Data can be read from the file. Combine with <code>FA_WRITE</code> for read-write access.</td>
    </tr>
    <tr class="even">
    <td>FA_WRITE</td>
    <td>Specifies write access to the object. Data can be written to the file. Combine with <code>FA_READ</code> for read-write access.</td>
    </tr>
    <tr class="odd">
    <td>FA_OPEN_EXISTING</td>
    <td>Opens the file. The function fails if the file is not existing. (Default)</td>
    </tr>
    <tr class="even">
    <td>FA_OPEN_ALWAYS</td>
    <td>Opens the file if it is existing. If not, a new file is created.<br />
    To append data to the file, use <a href="lseek.md"><code>f_lseek()</code></a> function after file open in this method.</td>
    </tr>
    <tr class="odd">
    <td>FA_CREATE_NEW</td>
    <td>Creates a new file. The function fails with <code>FR_EXIST</code> if the file is existing.</td>
    </tr>
    <tr class="even">
    <td>FA_CREATE_ALWAYS</td>
    <td>Creates a new file. If the file is existing, it will be truncated and overwritten.</td>
    </tr>
    </tbody>
    </table>

#### Return Values

[FR\_OK](rc.md#ok), [FR\_DISK\_ERR](rc.md#de),
[FR\_INT\_ERR](rc.md#ie), [FR\_NOT\_READY](rc.md#nr),
[FR\_NO\_FILE](rc.md#ok), [FR\_NO\_PATH](rc.md#np),
[FR\_INVALID\_NAME](rc.md#in), [FR\_DENIED](rc.md#de),
[FR\_EXIST](rc.md#ex), [FR\_INVALID\_OBJECT](rc.md#io),
[FR\_WRITE\_PROTECTED](rc.md#wp), [FR\_INVALID\_DRIVE](rc.md#id),
[FR\_NOT\_ENABLED](rc.md#ne), [FR\_NO\_FILESYSTEM](rc.md#ns),
[FR\_TIMEOUT](rc.md#tm), [FR\_LOCKED](rc.md#lo),
[FR\_NOT\_ENOUGH\_CORE](rc.md#nc),
[FR\_TOO\_MANY\_OPEN\_FILES](rc.md#tf)

#### Description

After `f_open()` function succeeded, the file object is valid. The file
object is used for subsequent read/write functions to identify the file.
To close an open file, use [`f_close()`](close.md) function. If the
file is modified and not closed properly, the file data will be
collapsed.

If duplicated file open is needed, read [here](appnote.md#dup)
carefully. However duplicated open of a file with write mode flag is
always prohibited.

Before using any file function, a work area (file system object) must be
registered to the logical drive with [`f_mount()`](mount.md) function.
All API functions except for [`f_fdisk()`](fdisk.md) function can work
after this procedure.

#### QuickInfo

Always available. The mode flags, `FA_WRITE, FA_CREATE_ALWAYS,
FA_CREATE_NEW and FA_OPEN_ALWAYS`, are not available when `_FS_READONLY
== 1`.

#### Example

    /* Read a text file and display it */
    
    FATFS FatFs;   /* Work area (file system object) for logical drive */
    
    int main (void)
    {
        FIL fil;       /* File object */
        char line[82]; /* Line buffer */
        FRESULT fr;    /* FatFs return code */
    
    
        /* Register work area to the default drive */
        f_mount(&FatFs, "", 0);
    
        /* Open a text file */
        fr = f_open(&fil, "message.txt", FA_READ);
        if (fr) return (int)fr;
    
        /* Read all lines and display it */
        while (f_gets(line, sizeof line, &fil))
            printf(line);
    
        /* Close the file */
        f_close(&fil);
    
        return 0;
    }

    /* Copy a file "file.bin" on the drive 1 to drive 0 */
    
    int main (void)
    {
        FATFS fs[2];         /* Work area (file system object) for logical drives */
        FIL fsrc, fdst;      /* File objects */
        BYTE buffer[4096];   /* File copy buffer */
        FRESULT fr;          /* FatFs function common result code */
        UINT br, bw;         /* File read/write count */
    
    
        /* Register work area for each logical drive */
        f_mount(&fs[0], "0:", 0);
        f_mount(&fs[1], "1:", 0);
    
        /* Open source file on the drive 1 */
        fr = f_open(&fsrc, "1:file.bin", FA_OPEN_EXISTING | FA_READ);
        if (fr) return (int)fr;
    
        /* Create destination file on the drive 0 */
        fr = f_open(&fdst, "0:file.bin", FA_CREATE_ALWAYS | FA_WRITE);
        if (fr) return (int)fr;
    
        /* Copy source to destination */
        for (;;) {
            fr = f_read(&fsrc, buffer, sizeof buffer, &br);  /* Read a chunk of source file */
            if (fr || br == 0) break; /* error or eof */
            fr = f_write(&fdst, buffer, br, &bw);            /* Write it to the destination file */
            if (fr || bw < br) break; /* error or disk full */
        }
    
        /* Close open files */
        f_close(&fsrc);
        f_close(&fdst);
    
        /* Unregister work area prior to discard it */
        f_mount(NULL, "0:", 0);
        f_mount(NULL, "1:", 0);
    
        return (int)fr;
    }

#### See Also

`f_read, f_write, f_close, FIL, FATFS`
