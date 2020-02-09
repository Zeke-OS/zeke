## f\_putc

The f\_putc funciton puts a character to the file.

    int f_putc (
      TCHAR chr,  /* [IN] A character to put */
      FIL* fp     /* [IN] File object */
    );

#### Parameters

  - chr  
    A character to be put.
  - fp  
    Pointer to the open file object structuer.

#### Return Values

When the character was written successfuly, it returns number of
characters written. When the function failed due to disk full or any
error, an `EOF (-1)` will be returned.

When FatFs is configured to Unicode API (`_LFN_UNICODE == 1`), character
encoding on the string fuctions, `f_putc()`, `f_puts()`, `f_printf()`
and `f_gets()`, is also switched to Unicode. The character encoding on
the file to be read/written via those functions is selected by
`_STRF_ENCODE` option.

#### Description

The `f_putc()` function is a wrapper function of
[`f_write()`](write.md) function.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_USE_STRFUNC` is 1 or 2. When it
is set to 2, a `'\n'` is converted to `'\r'+'\n'`.

#### See Also

`f_open, f_puts, f_printf, f_gets, f_close, FIL`
