## f\_eof

The f\_eof function tests for end-of-file on a file.

    int f_eof (
      FIL* fp   /* [IN] File object */
    );

#### Parameters

  - fp  
    Pointer to the open file object structure.

#### Return Values

The `f_eof()` function returns a non-zero value if the read/write
pointer has reached end of the file; otherwise it returns a zero.

#### Description

In this revision, this function is implemented as a macro.

    #define f_eof(fp) (((fp)->fptr) == ((fp)->fsize) ? 1 : 0)

#### QuickInfo

Always available.

#### See Also

`f_open, f_lseek, FIL`
