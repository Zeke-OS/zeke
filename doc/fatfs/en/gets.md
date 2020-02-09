## f\_gets

The f\_gets reads a string from the file.

    TCHAR* f_gets (
      TCHAR* buff, /* [OUT] Read buffer */
      int len,     /* [IN] Size of the read buffer */
      FIL* fp      /* [IN] File object */
    );

#### Parameters

  - buff  
    Pointer to read buffer to store the read string.
  - len  
    Size of the read buffer in unit of character.
  - fp  
    Pointer to the open file object structure.

#### Return Values

When the function succeeded, `buff` will be returuned.

#### Description

The `f_gets()` function is a wrapper function of [`f_read()`](read.md)
function. The read operation continues until a `'\n'` is stored, reached
end of the file or the buffer is filled with `len - 1` characters. The
read string is terminated with a `'\0'`. When no character to read or
any error occured during read operation, it returns a null pointer. The
status of EOF and error can be examined with `f_eof()` and `f_error()`
macros.

When FatFs is configured to Unicode API (`_LFN_UNICODE == 1`), data
types on the srting fuctions, `f_putc()`, `f_puts()`, `f_printf()` and
`f_gets()`, is also switched to Unicode. The character encoding on the
file to be read/written via those functions is selected by
`_STRF_ENCODE` option.

#### QuickInfo

Available when `_USE_STRFUNC` is 1 or 2. When it is set to 2, `'\r'`s
contained in the file are stripped out.

#### See Also

`f_open, f_read, f_putc, f_puts, f_printf, f_close, FIL`
