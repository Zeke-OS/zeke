## f\_tell

The f\_tell function gets the current read/write pointer of a file.

```c
DWORD f_tell (
    FIL* fp   /* [IN] File object */
);
```

#### Parameters

  - fp  
    Pointer to the open file object structure.

#### Return Values

Returns current read/write pointer of the file.

#### Description

In this revision, the `f_tell()` function is implemented as a macro.

    #define f_tell(fp) ((fp)->fptr)

#### QuickInfo

Always available.

#### See Also

`f_open, f_lseek, FIL`
