/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2013          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#define _USE_WRITE  1   /* 1: Enable disk_write function */
#define _USE_IOCTL  1   /* 1: Enable disk_ioctl fucntion */

#include "integer.h"


/* Results of Disk Functions */
typedef enum {
    RES_OK = 0,     /* 0: Successful */
    RES_ERROR,      /* 1: R/W Error */
    RES_WRPRT,      /* 2: Write Protected */
    RES_NOTRDY,     /* 3: Not Ready */
    RES_PARERR      /* 4: Invalid Parameter */
} DRESULT;

/* Prototypes for disk control functions */
DRESULT fatfs_disk_read(uint8_t pdrv, uint8_t * buff, DWORD sector,
                        unsigned int count);
DRESULT fatfs_disk_write(uint8_t pdrv, const uint8_t * buff, DWORD sector,
                         unsigned int count);
DRESULT fatfs_disk_ioctl(uint8_t pdrv, unsigned cmd, void * buff, size_t bsize);

/* Generic command (used by FatFs) */
#define CTRL_SYNC           0 /*!< Flush disk cache (for write functions) */
#define GET_BLOCK_SIZE      3 /*!< Get erase block size (for only f_mkfs()) */
#define CTRL_ERASE_SECTOR   4 /*!< Force erased a block of sectors
                               *   (for only _USE_ERASE) */

#endif
