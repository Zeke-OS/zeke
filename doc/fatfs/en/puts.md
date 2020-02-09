## f\_puts

The f\_puts function writes a string to the file.

    int f_puts (
      const TCHAR* str, /* [IN] String */
      FIL* fp           /* [IN] File object */
    );

#### Parameters

  - str  
    Pointer to the null terminated string to be written. The terminator
    character will not be written.
  - fp  
    Pointer to the open file object structure.

#### Return Value

When the function succeeded, it returns number of characters written.
When the write operation is aborted due to disk full or any error, an
`EOF (-1)` will be returned.

When FatFs is configured to Unicode API (`_LFN_UNICODE == 1`), character
encoding on the srting fuctions, `f_putc()`, `f_puts()`, `f_printf()`
and `f_gets()`, is also switched to Unicode. The character encoding on
the file to be read/written via those functions is selected by
`_STRF_ENCODE` option.

#### Description

The `f_puts()` function is a wrapper function of
[`f_write()`](write.md) function.

#### QuickInfo

Available when `_FS_READONLY == 0` and `_USE_STRFUNC` is 1 or 2. When it
is set to 2, `'\n'`s contained in the string are converted to
`'\r'+'\n'`.

#### See Also

`f_open, f_putc, f_printf, f_gets, f_close, FIL`
