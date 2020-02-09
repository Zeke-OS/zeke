## get\_fattime

The get\_fattime function gets current time.

    DWORD get_fattime (void);

#### Return Value

Currnet time is returned with packed into a `DWORD` value. The bit field
is as follows:

  - bit31:25  
    Year origin from the 1980 (0..127)
  - bit24:21  
    Month (1..12)
  - bit20:16  
    Day of the month(1..31)
  - bit15:11  
    Hour (0..23)
  - bit10:5  
    Minute (0..59)
  - bit4:0  
    Second / 2 (0..29)

#### Description

The `get_fattime()` function shall return any valid time even if the
system does not support a real time clock. If a zero is returned, the
file will not have a valid timestamp.

#### QuickInfo

This function is not needed when `_FS_READONLY == 1`.
