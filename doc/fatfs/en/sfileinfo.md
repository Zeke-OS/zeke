## FILINFO

The `FILINFO` structure holds a file information returned by
`f_readdir()` and `f_stat()` function.

```c
typedef struct {
    DWORD fsize;      /* File size */
    WORD  fdate;      /* Last modified date */
    WORD  ftime;      /* Last modified time */
    BYTE  fattrib;    /* Attribute */
    TCHAR fname[13];  /* Short file name (8.3 format) */
#if _USE_LFN
    TCHAR* lfname;    /* Pointer to the LFN buffer */
    int   lfsize;     /* Size of the LFN buffer in unit of TCHAR */
#endif
} FILINFO;
```

#### Members

  - fsize  
    Indicates size of the file in unit of byte. Always zero for
    directories.
  - fdate  
    Indicates the date that the file was modified or the directory was
    created.  
      - bit15:9  
        Year origin from 1980 (0..127)
      - bit8:5  
        Month (1..12)
      - bit4:0  
        Day (1..31)
  - ftime  
    Indicates the time that the file was modified or the directory was
    created.  
      - bit15:11  
        Hour (0..23)
      - bit10:5  
        Minute (0..59)
      - bit4:0  
        Second / 2 (0..29)
  - fattrib  
    Indicates the file/directory attribute in combination of `AM_DIR`,
    `AM_RDO`, `AM_HID`, `AM_SYS` and `AM_ARC`.
  - fname\[\]  
    Indicates the file/directory name in 8.3 format null-terminated
    string. It is always returnd with upper case in non-LFN
    configuration but it can be returned with lower case in LFN
    configuration.
  - lfname  
    Pointer to the LFN buffer to store the read LFN. This member must be
    initialized by application prior to use this structure. Set null
    pointer if LFN is not needed. Not available in non-LFN
    configuration.
  - lfsize  
    Size of the LFN buffer in unit of TCHAR. This member must be
    initialized by application prior to use this structure. Not
    available in non-LFN configuration.
