## f\_size

The f\_size function gets the size of a file.

    DWORD f_size (
      FIL* fp   /* [IN] File object */
    );

#### Parameters

  - fp  
    Pointer to the open file object structure.

#### Return Values

Returns the size of the file in unit of byte.

#### Description

In this revision, the `f_size()` function is implemented as a macro.

    #define f_size(fp) ((fp)->fsize)

#### QuickInfo

Always available.

#### See Also

`f_open, f_lseek, FIL`
