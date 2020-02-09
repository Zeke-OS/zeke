## f\_error

The f\_error tests for an error on a file.

    int f_error (
      FIL* fp   /* [IN] File object */
    );

#### Parameters

  - fp  
    Pointer to the open file object structure.

#### Return Values

Returns a non-zero value if a hard error has occured; otherwise it
returns a zero.

#### Description

In this revision, this function is implemented as a macro.

    #define f_error(fp) (((fp)->flag & FA__ERROR) ? 1 : 0)

#### QuickInfo

Always available.

#### See Also

`f_open, FIL`
