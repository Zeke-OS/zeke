#include <klocks.h>

/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module configuration file  R0.10b (C)ChaN, 2014
/---------------------------------------------------------------------------*/

#ifndef _FFCONF
#define _FFCONF


/*---------------------------------------------------------------------------/
/ Functions and Buffer Configurations
/---------------------------------------------------------------------------*/

/**
 * To enable fast seek feature, set _USE_FASTSEEK to 1.
 * 0:Disable or 1:Enable
 */
#define _USE_FASTSEEK   0

/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

/**
 * The _CODE_PAGE specifies the OEM code page to be used on the target system.
 * Incorrect setting of the code page can cause a file open failure.
 *
 *  932  - Japanese Shift_JIS (DBCS, OEM, Windows)
 *  936  - Simplified Chinese GBK (DBCS, OEM, Windows)
 *  949  - Korean (DBCS, OEM, Windows)
 *  950  - Traditional Chinese Big5 (DBCS, OEM, Windows)
 *  1250 - Central Europe (Windows)
 *  1251 - Cyrillic (Windows)
 *  1252 - Latin 1 (Windows)
 *  1253 - Greek (Windows)
 *  1254 - Turkish (Windows)
 *  1255 - Hebrew (Windows)
 *  1256 - Arabic (Windows)
 *  1257 - Baltic (Windows)
 *  1258 - Vietnam (OEM, Windows)
 *  437  - U.S. (OEM)
 *  720  - Arabic (OEM)
 *  737  - Greek (OEM)
 *  775  - Baltic (OEM)
 *  850  - Multilingual Latin 1 (OEM)
 *  858  - Multilingual Latin 1 + Euro (OEM)
 *  852  - Latin 2 (OEM)
 *  855  - Cyrillic (OEM)
 *  866  - Russian (OEM)
 *  857  - Turkish (OEM)
 *  862  - Hebrew (OEM)
 *  874  - Thai (OEM, Windows)
 *  1    - ASCII (Valid for only non-LFN configuration)
 */
#define _CODE_PAGE configFATFS_CODEPAGE


/**
 * Maximum LFN length to handle (12 to 255)
 * When LFN feature is enabled, Unicode handling functions ff_convert() and
 * ff_wtoupper() function must be added to the project.
 */
#define _MAX_LFN 255


/**
 * 0:ANSI/OEM or 1:Unicode
 * To switch the character encoding on the FatFs API (TCHAR) to Unicode,
 * enable LFN feature and set _LFN_UNICODE to 1. This option affects behavior
 * of string I/O functions. This option must be 0 when LFN feature is not
 * enabled.
 */
#ifdef configFATFS_UNICODE
#define _LFN_UNICODE 1
#else
#define _LFN_UNICODE 0
#endif


/**
 * When Unicode API is enabled by _LFN_UNICODE option, this option selects the
 * character encoding on the file to be read/written via string I/O functions,
 * f_gets(), f_putc(), f_puts and f_printf(). This option has no effect when
 * _LFN_UNICODE == 0. Note that FatFs supports only BMP.
 * 0:ANSI/OEM, 1:UTF-16LE, 2:UTF-16BE, 3:UTF-8
 */
#define _STRF_ENCODE    3


/*---------------------------------------------------------------------------/
 * Drive/Volume Configurations
 *--------------------------------------------------------------------------*/

/**
 * Number of volumes (logical drives) to be used.
 */
#define _VOLUMES configFATFS_MAX_MOUNTS


/**
 * By default(0), each logical drive number is bound to the same physical drive
 * number and only a FAT volume found on the physical drive is mounted. When it
 * is set to 1,
 * each logical drive number is bound to arbitrary drive/partition listed in
 * VolToPart[].
 * 0:Single partition, 1:Enable multiple partition
 */
#define _MULTI_PARTITION 0


/**
 * These options configure the range of sector size to be supported. (512, 1024,
 * 2048 or 4096) Always set both 512 for most systems, all memory card and
 * harddisk. But a larger value may be required for on-board flash memory and
 * some type of optical media. When _MAX_SS is larger than _MIN_SS, FatFs is
 * configured to variable sector size and GET_SECTOR_SIZE command must be
 * implemented to the disk_ioctl() function.
 */
#define _MIN_SS     512
#define _MAX_SS     4096


/**
 * To enable sector erase feature, set _USE_ERASE to 1. Also CTRL_ERASE_SECTOR
 * command should be added to the disk_ioctl() function.
 * 0:Disable or 1:Enable
 */
#define _USE_ERASE 0


/**
 * If you need to know correct free space on the FAT32 volume, set bit 0
 * of this option and f_getfree() function at first time after volume mount
 * will force a full FAT scan. Bit 1 controls the last allocated cluster number
 * as bit 0.
 *
 * 0 to 3
 *
 * bit0=0: Use free cluster count in the FSINFO if available.
 * bit0=1: Do not trust free cluster count in the FSINFO.
 * bit1=0: Use last allocated cluster number in the FSINFO if available.
 * bit1=1: Do not trust last allocated cluster number in the FSINFO.
 */
#define _FS_NOFSINFO 0



/*---------------------------------------------------------------------------/
/ System Configurations
/---------------------------------------------------------------------------*/

/**
 * Fatfs reentrancy timeout.
 * Timeout period in unit of time tick
 */
#define _FS_TIMEOUT     1000

/**
 * The _WORD_ACCESS option is an only platform dependent option. It defines
 * which access method is used to the word data on the FAT volume.
 *
 *  0: Byte-by-byte access. Always compatible with all platforms.
 *  1: Word access. Do not choose this unless under both the following
 *     conditions.
 *
 * Address misaligned memory access is always allowed for ALL instructions.
 * Byte order on the memory is little-endian.
 *
 * If it is the case, _WORD_ACCESS can also be set to 1 to improve performance
 * and reduce code size. Following table shows an example of some processor
 * types.
 *
 * 0 or 1
 *
 *  ARM7TDMI    0           ColdFire    0           V850E       0
 *  Cortex-M3   0           Z80         0/1         V850ES      0/1
 *  Cortex-M0   0           RX600(LE)   0/1         TLCS-870    0/1
 *  AVR         0/1         RX600(BE)   0           TLCS-900    0/1
 *  AVR32       0           RL78        0           R32C        0
 *  PIC18       0/1         SH-2        0           M16C        0/1
 *  PIC24       0           H8S         0           MSP430      0
 *  PIC32       0           H8/300H     0           x86         0/1
 */
#define _WORD_ACCESS    0

#endif /* _FFCONF */
