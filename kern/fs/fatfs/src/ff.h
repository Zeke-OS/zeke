/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module include file  R0.10b    (C)ChaN, 2014
/----------------------------------------------------------------------------/
/ FatFs module is a generic FAT file system module for small embedded systems.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following terms.
/
/  Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
/  Copyright (C) 2014, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial product UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/----------------------------------------------------------------------------*/

#pragma once
#ifndef _FATFS
#define _FATFS

__BEGIN_DECLS

#include <time.h>
#include <klocks.h>
#include "integer.h"    /* Basic integer types */
#include "ffconf.h"     /* FatFs configuration options */

/*
 * Type of path name strings on FatFs API
 */
#if _LFN_UNICODE            /* Unicode string */
#ifndef configFATFS_LFN
#error _LFN_UNICODE must be 0 at non-LFN cfg.
#endif
#ifndef _INC_TCHAR
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x
#endif

#else                       /* ANSI/OEM string */
#ifndef _INC_TCHAR
typedef char TCHAR;
#define _T(x) x
#define _TEXT(x) x
#endif

#endif

#define FATFS_READONLY  0x01

#define LFN_SIZE        (NAME_MAX + 1)

/**
 * File system object structure (FATFS)
 */
typedef struct {
    uint8_t fs_type;        /* FAT sub-type (0:Not mounted) */
    uint8_t drv;            /* Physical drive number */
    uint8_t csize;          /* Sectors per cluster (1,2,4...128) */
    uint8_t n_fats;         /* Number of FAT copies (1 or 2) */
    uint8_t wflag;          /* win[] flag (b0:dirty) */
    uint8_t fsi_flag;       /* FSINFO flags (b7:disabled, b0:dirty) */
    WORD    id;             /* File system mount ID */
    WORD    n_rootdir;      /* Number of root directory entries (FAT12/16) */
    WORD    ssize;          /* uint8_ts per sector (512, 1024, 2048 or 4096) */
    mtx_t   sobj;           /* Identifier of sync object */
    unsigned opt;           /* fs mount options */
    DWORD   last_clust;     /* Last allocated cluster */
    DWORD   free_clust;     /* Number of free clusters */
    DWORD   n_fatent;       /* Number of FAT entries, = number of clusters + 2 */
    DWORD   fsize;          /* Sectors per FAT */
    DWORD   fatbase;        /* FAT start sector */
    DWORD   dirbase;        /* Root directory start sector (FAT32:Cluster#) */
    DWORD   database;       /* Data start sector */
    DWORD   winsect;        /* Current sector appearing in the win[] */
    uint8_t win[_MAX_SS];   /* Disk access window for Directory, FAT (and file data at tiny cfg) */
} FATFS;



/* File object structure (FIL) */

typedef struct {
    FATFS*  fs;             /* Pointer to the related file system object (**do not change order**) */
    uint64_t ino;           /* Emulated ino */
    uint8_t flag;           /* Status flags */
    uint8_t err;            /* Abort flag (error code) */
    DWORD   fptr;           /* File read/write pointer (Zeroed on file open) */
    DWORD   fsize;          /* File size */
    DWORD   sclust;         /* File start cluster (0:no cluster chain, always 0 when fsize is 0) */
    DWORD   clust;          /* Current cluster of fpter (not valid when fprt is 0) */
    DWORD   dsect;          /* Sector number appearing in buf[] (0:invalid) */
    DWORD   dir_sect;       /* Sector number containing the directory entry */
    uint8_t * dir_ptr;      /* Pointer to the directory entry in the win[] */
#if _USE_FASTSEEK
    DWORD * cltbl;          /* Pointer to the cluster link map table (Nulled on file open) */
#endif
    uint8_t buf[_MAX_SS];   /* File private data read/write window */
} FF_FIL;



/* Directory object structure (DIR) */

typedef struct {
    FATFS*  fs;             /* Pointer to the owner file system object (**do not change order**) */
    uint64_t ino;           /* Emulated ino */
    WORD    index;          /* Current read/write index number */
    DWORD   sclust;         /* Table start cluster (0:Root dir) */
    DWORD   clust;          /* Current cluster */
    DWORD   sect;           /* Current sector */
    uint8_t * dir;          /* Pointer to the current SFN entry in the win[] */
    uint8_t * fn;           /* Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
#if configFATFS_LFN
    WCHAR * lfn;            /* Pointer to the LFN working buffer */
    WORD    lfn_idx;        /* Last matched LFN index number (0xFFFF:No LFN) */
#endif
} FF_DIR;



/* File status structure (FILINFO) */

typedef struct {
    DWORD   fsize;          /* File size */
    struct timespec fatime; /* Access time. */
    struct timespec fmtime; /* Modification time. */
    struct timespec fbtime; /* Creation time. */
    uint8_t fattrib;        /* Attribute */
    uint64_t ino;           /* Emulated ino */
    TCHAR   fname[13];      /* Short file name (8.3 format) */
#if configFATFS_LFN
    TCHAR * lfname;         /* Pointer to the LFN buffer */
#endif
} FILINFO;



/* File function return code (FRESULT) */

typedef enum {
    FR_OK = 0,              /* (0) Succeeded */
    FR_DISK_ERR,            /* (1) A hard error occurred in the low level disk I/O layer */
    FR_INT_ERR,             /* (2) Assertion failed */
    FR_NOT_READY,           /* (3) The physical drive cannot work */
    FR_NO_FILE,             /* (4) Could not find the file */
    FR_NO_PATH,             /* (5) Could not find the path */
    FR_INVALID_NAME,        /* (6) The path name format is invalid */
    FR_DENIED,              /* (7) Access denied due to prohibited access or directory full */
    FR_EXIST,               /* (8) Access denied due to prohibited access */
    FR_INVALID_OBJECT,      /* (9) The file/directory object is invalid */
    FR_WRITE_PROTECTED,     /* (10) The physical drive is write protected */
    FR_INVALID_DRIVE,       /* (11) The logical drive number is invalid */
    FR_NOT_ENABLED,         /* (12) The volume has no work area */
    FR_NO_FILESYSTEM,       /* (13) There is no valid FAT volume */
    FR_MKFS_ABORTED,        /* (14) The f_mkfs() aborted due to any parameter error */
    FR_TIMEOUT,             /* (15) Could not get a grant to access the volume within defined period */
    FR_LOCKED,              /* (16) The operation is rejected according to the file sharing policy */
    FR_NOT_ENOUGH_CORE,     /* (17) LFN working buffer could not be allocated */
    FR_TOO_MANY_OPEN_FILES, /* (18) Number of open files > _FS_SHARE */
    FR_INVALID_PARAMETER    /* (19) Given parameter is invalid */
} FRESULT;

/*--------------------------------------------------------------*/
/* FatFs module application interface                           */

FRESULT f_open(FF_FIL * fp, FATFS * fs, const TCHAR * path, uint8_t mode);
FRESULT f_read(FF_FIL * fp, void * buff, unsigned int btr, unsigned int * br);
FRESULT f_write(FF_FIL * fp, const void * buff, unsigned int btw,
                unsigned int * bw);
FRESULT f_lseek(FF_FIL * fp, DWORD ofs);
FRESULT f_truncate(FF_FIL * fp);
FRESULT f_sync(FF_FIL * fp, int validated);
FRESULT f_opendir(FF_DIR * dp, FATFS * fs, const TCHAR * path);
FRESULT f_readdir(FF_DIR * dp, FILINFO * fno);
FRESULT f_mkdir(FATFS * fs, const TCHAR * path);
FRESULT f_unlink(FATFS * fs, const TCHAR * path);
FRESULT f_rename(FATFS * fs, const TCHAR * path_old, const TCHAR * path_new);
FRESULT f_stat(FATFS * fs, const TCHAR * path, FILINFO * fno);
FRESULT f_chmod(FATFS * fs, const TCHAR * path, uint8_t value, uint8_t mask);
FRESULT f_utime(FATFS * fs, const TCHAR * path, const struct timespec * ts);
FRESULT f_chdir(const TCHAR * path);
FRESULT f_chdrive(const TCHAR * path);
FRESULT f_getfree(FATFS * fs, DWORD * nclst);
FRESULT f_getlabel(FATFS * fs, const TCHAR * path, TCHAR * label, DWORD * vsn);
FRESULT f_setlabel(FATFS * fs, const TCHAR * label);
FRESULT f_mount(FATFS * fs, int vol, uint8_t opt);
FRESULT f_umount(FATFS * fs);

#define f_eof(fp) (((fp)->fptr == (fp)->fsize) ? 1 : 0)
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->fsize)

#ifndef EOF
#define EOF (-1)
#endif


/*--------------------------------------------------------------*/
/* Additional user defined functions                            */

/* RTC function */
void fatfs_time_fat2unix(struct timespec * ts, uint32_t dt, int tenth);
uint32_t fatfs_time_unix2fat(const struct timespec * ts);
uint32_t fatfs_time_get_time(void);

/* Unicode support functions */
#if configFATFS_LFN                     /* Unicode - OEM code conversion */
WCHAR ff_convert(WCHAR chr, unsigned int dir); /* OEM-Unicode bidirectional conversion */
WCHAR ff_wtoupper(WCHAR chr);           /* Unicode upper-case conversion */
                                        /* Memory functions */
void* ff_memalloc(unsigned int msize);          /* Allocate memory block */
void ff_memfree(void* mblock);          /* Free memory block */
#endif


/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File access control and file status flags (FIL.flag) */

#define FA_READ             0x01
#define FA_OPEN_EXISTING    0x00
#define FA_WRITE            0x02
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA__WRITTEN         0x20
#define FA__DIRTY           0x40


/* FAT sub type (FATFS.fs_type) */

#define FS_FAT12    1
#define FS_FAT16    2
#define FS_FAT32    3


/* File attribute bits for directory entry */

#define AM_RDO  0x01    /* Read only */
#define AM_HID  0x02    /* Hidden */
#define AM_SYS  0x04    /* System */
#define AM_VOL  0x08    /* Volume label */
#define AM_LFN  0x0F    /* LFN entry */
#define AM_DIR  0x10    /* Directory */
#define AM_ARC  0x20    /* Archive */
#define AM_MASK 0x3F    /* Mask of defined bits */


/* Fast seek feature */
#define CREATE_LINKMAP  0xFFFFFFFF

/*--------------------------------*/
/* Multi-byte word access macros  */

#if _WORD_ACCESS == 1   /* Enable word access to the FAT structure */
#define LD_WORD(ptr)        (WORD)(*(WORD*)(uint8_t*)(ptr))
#define LD_DWORD(ptr)       (DWORD)(*(DWORD*)(uint8_t*)(ptr))
#define ST_WORD(ptr,val)    *(WORD*)(uint8_t*)(ptr)=(WORD)(val)
#define ST_DWORD(ptr,val)   *(DWORD*)(uint8_t*)(ptr)=(DWORD)(val)
#else                   /* Use byte-by-byte access to the FAT structure */
#define LD_WORD(ptr)        (WORD)(((WORD)*((uint8_t*)(ptr)+1)<<8)|(WORD)*(uint8_t*)(ptr))
#define LD_DWORD(ptr)       (DWORD)(((DWORD)*((uint8_t*)(ptr)+3)<<24)|((DWORD)*((uint8_t*)(ptr)+2)<<16)|((WORD)*((uint8_t*)(ptr)+1)<<8)|*(uint8_t*)(ptr))
#define ST_WORD(ptr,val)    *(uint8_t*)(ptr)=(uint8_t)(val); *((uint8_t*)(ptr)+1)=(uint8_t)((WORD)(val)>>8)
#define ST_DWORD(ptr,val)   *(uint8_t*)(ptr)=(uint8_t)(val); *((uint8_t*)(ptr)+1)=(uint8_t)((WORD)(val)>>8); *((uint8_t*)(ptr)+2)=(uint8_t)((DWORD)(val)>>16); *((uint8_t*)(ptr)+3)=(uint8_t)((DWORD)(val)>>24)
#endif

__END_DECLS

#endif /* _FATFS */
