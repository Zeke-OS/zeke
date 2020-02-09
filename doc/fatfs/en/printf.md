## f\_printf

The f\_printf function writes formatted string to the file.

    int f_printf (
      FIL* fp,          /* [IN] File object */
      const TCHAR* fmt, /* [IN] Format stirng */
      ...
    );

#### Parameters

  - fp  
    Pointer to the open file object structure.
  - fmt  
    Pointer to the null terminated format string. The terminator
    charactor will not be written.
  - ...  
    Optional arguments...

#### Return Values

When the function succeeded, it returns number of characters written.
When the function failed due to disk full or any error, an `EOF (-1)`
will be returned.

#### Description

The `f_printf()` is a wrapper function of [`f_write()`](write.md). The
format control directive is a sub-set of standard library shown as
follos:

  - Type: `c C s S d D u U x X b B`
  - Size: `l L`
  - Flag: `0 -`

#### QuickInfo

Available when `_FS_READONLY == 0` and `_USE_STRFUNC` is 1 or 2. When it
is set to 2, `'\n'`s contained in the output are converted to
`'\r'+'\n'`.

When FatFs is configured to Unicode API (`_LFN_UNICODE == 1`), data
types on the srting fuctions, `f_putc()`, `f_puts()`, `f_printf()` and
`f_gets()`, is also switched to Unicode. The character encoding on the
file to be read/written via those functions is selected by
`_STRF_ENCODE` option.

#### Example

``` 
    f_printf(&fil, "%d", 1234);            /* "1234" */
    f_printf(&fil, "%6d,%3d%%", -200, 5);  /* "  -200,  5%" */
    f_printf(&fil, "%ld", 12345L);         /* "12345" */
    f_printf(&fil, "%06d", 25);            /* "000025" */
    f_printf(&fil, "%06d", -25);           /* "000-25" */
    f_printf(&fil, "%-6d", 25);            /* "25    " */
    f_printf(&fil, "%u", -1);              /* "65535" or "4294967295" */
    f_printf(&fil, "%04x", 0xAB3);         /* "0ab3" */
    f_printf(&fil, "%08LX", 0x123ABCL);    /* "00123ABC" */
    f_printf(&fil, "%016b", 0x550F);       /* "0101010100001111" */
    f_printf(&fil, "%s", "String");        /* "String" */
    f_printf(&fil, "%8s", "abc");          /* "     abc" */
    f_printf(&fil, "%-8s", "abc");         /* "abc     " */
    f_printf(&fil, "%c", 'a');             /* "a" */
    f_printf(&fil, "%f", 10.0);            /* f_printf lacks floating point support */
```

#### See Also

`f_open, f_putc, f_puts, f_gets, f_close, FIL`
