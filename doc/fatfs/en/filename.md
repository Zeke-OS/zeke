# Path Names

### Format of the path names

The format of path name on the FatFs module is similer to the filename
specs of DOS/Windos as follows:

    "[drive:][/]directory/file"

The FatFs module supports long file name (LFN) and 8.3 format file name
(SFN). The LFN can be used when LFN feature is enabled (`_USE_LFN > 0`).
The sub directories are separated with a \\ or / in the same way as
DOS/Windows API. Duplicated separators are skipped and ignored. Only a
difference is that the logical drive is specified in a numeral with a
colon. When drive number is omitted, the drive number is assumed as
*default drive* (drive 0 or current drive).

Control characters (`'\0'` to `'\x1F'`) are recognized as end of the
path name. Leading/embedded spaces in the path name are valid as a part
of the name at LFN configuration but they are recognized as end of the
path name at non-LFN configuration. Trailing spaces and dots are
ignored.

In default configuration (`_FS_RPATH == 0`), it does not have a concept
of current directory like OS oriented file system. All objects on the
volume are always specified in full path name that follows from the root
directory. Dot directory names are not allowed. Heading separator is
ignored and it can be exist or omitted. The default drive is fixed to
drive 0.

When relative path feature is enabled (`_FS_RPATH == 1`), specified path
is followed from the root directory if a heading separator is exist. If
not, it is followed from the current directory of the drive set with
[f\_chdir](chdir.md) function. Dot names are also allowed for the path
name. The default drive is the current drive set with
[f\_chdrive](chdrive.md)
function.

|             |                                             |                                                      |
| ----------- | ------------------------------------------- | ---------------------------------------------------- |
| Path name   | \_FS\_RPATH == 0                            | \_FS\_RPATH == 1                                     |
| file.txt    | A file in the root directory of the drive 0 | A file in the current directory of the current drive |
| /file.txt   | A file in the root directory of the drive 0 | A file in the root directory of the current drive    |
|             | The root directory of the drive 0           | The current directory of the current drive           |
| /           | The root directory of the drive 0           | The root directory of the current drive              |
| 2:          | The root directory of the drive 2           | The current directory of the drive 2                 |
| 2:/         | The root directory of the drive 2           | The root directory of the drive 2                    |
| 2:file.txt  | A file in the root directory of the drive 2 | A file in the current directory of the drive 2       |
| ../file.txt | Invalid name                                | A file in the parent directory                       |
| .           | Invalid name                                | This directory                                       |
| ..          | Invalid name                                | Parent directory of the current directory            |
| dir1/..     | Invalid name                                | The current directory                                |
| /..         | Invalid name                                | The root directory (sticks the top level)            |

When option `_STR_VOLUME_ID` is specified, also pre-defined strings can
be used as drive identifier in the path name instead of a numeral.

### Unicode API

The path names are input/output in either ANSI/OEM code (SBCS/DBCS) or
Unicode depends on the configuration options. The type of arguments
which specify the file names are defined as `TCHAR`. It is an alias of
`char` in default. The code set used to the file name string is ANSI/OEM
specifid by `_CODE_PAGE`. When `_LFN_UNICODE` is set to 1, the type of
the `TCHAR` is switched to `WCHAR` to support Unicode (UTF-16 encoding).
In this case, the LFN feature is fully supported and the Unicode
specific characters, such as ✝☪✡☸☭, can also be used for the path name.
It also affects data types and encoding of the string I/O functions. To
define literal strings, `_T(s)` and `_TEXT(s)` macro are available to
select either ANSI/OEM or Unicode automatically. The code shown below is
an example to define the literal strings.

``` 
 f_open(fp, "filename.txt", FA_READ);      /* ANSI/OEM string */
 f_open(fp, L"filename.txt", FA_READ);     /* Unicode string */
 f_open(fp, _T("filename.txt"), FA_READ);  /* Changed by configuration */
```

### Volume Management

The FatFs module needs dynamic work area called *file system object* for
each volume (logical drive). It is registered to the FatFs module by
`f_mount()` function. By default, each logical drive is bound to the
physical drive with the same drive number and an FAT volume on the drive
is serched by auto detect feature. It loads boot sectors and checks it
if it is an FAT boot sector in order of sector 0 as SFD format, 1st
partition, 2nd partition, 3rd partition and 4th partition as FDISK
format.

When `_MULTI_PARTITION == 1` is specified by configuration option, each
individual logical drive is bound to the partition on the physical drive
specified by volume management table. The volume management table must
be defined by user to resolve relationship between logical drives and
partitions. Following code is an example of a volume management
table.

``` 
Example: Logical drive 0-2 are tied to three pri-partitions on the physical drive 0 (fixed disk)
         Logical drive 3 is tied to an FAT volume on the physical drive 1 (removable disk)

PARTITION VolToPart[] = {
    {0, 1},     /* Logical drive 0 ==> Physical drive 0, 1st partition */
    {0, 2},     /* Logical drive 1 ==> Physical drive 0, 2nd partition */
    {0, 3},     /* Logical drive 2 ==> Physical drive 0, 3rd partition */
    {1, 0}      /* Logical drive 3 ==> Physical drive 1 (auto detection) */
};

```

There are some considerations on using `_MULTI_PARTITION` configuration.

  - Only four primary partitions can be specified. Logical partition is
    not supported.
  - The physical drive that has two or more mounted partitions must be
    non-removable. Media change while a system operation is prohibited.
