/**
 *******************************************************************************
 * @file    devtypes.h
 * @author  Olli Vanhoja
 * @brief   Device driver types header file.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

#pragma once
#ifndef DEVTYPES_H
#define DEVTYPES_H

#include <stdint.h>

typedef uint32_t osDev_t; /*!< Device identifier */

#define DEV_MINORBITS   27
#define DEV_MINORMASK   ((1u << MINORBITS) - 1)

/**
 * Get major number from osDev_t.
 */
#define DEV_MAJOR(dev)  ((unsigned int)((dev) >> DEV_MINORBITS))

/**
 * Get minor number from isDev_t.
 */
#define DEV_MINOR(dev)  ((unsigned int)((dev) & DEV_MINORMASK))

/**
 * Convert major, minor pair into osDev_t.
 */
#define DEV_MMTODEV(ma, mi) (((ma) << DEV_MINORBITS) | (mi))

/* Common error codes */
#define DEV_COME_OK         0x00 /*!< No error, OK. */
#define DEV_COME_NLOCK      0x10 /*!< No lock acquired for the device or not locked. */
#define DEV_COME_INTERNAL   0x20 /*!< Device driver internal error. */
#define DEV_COME_EOF        0x30 /*!< End of file. */
#define DEV_COME_NDEV       0x40 /*!< Not a device of expected type. */
#define DEV_COME_BUSY       0x50 /*!< Device busy. */

/* Device lock & open error codes */
#define DEV_OERR_OK         DEV_COME_OK /*!< No error, OK. */
#define DEV_OERR_LOCKED     1  /*!< Device is locked by another thread. */
#define DEV_OERR_NONLOCK    2  /*!< Device is non-lockable. */
#define DEV_OERR_INTERNAL   DEV_COME_INTERNAL /*!< Device driver internal error. */
#define DEV_OERR_UNKNOWN    3 /*!< Unknown device/No driver for this device. */

/* Device close error codes */
#define DEV_CERR_OK         DEV_COME_OK /*!< No error, OK. */
#define DEV_CERR_NLOCK      DEV_COME_NLOCK /*!< Device not reserved for this thread. */
#define DEV_CERR_ULOCK      4 /*!< Unable to unlock/Device driver is reserved. */

/* Character device write error codes */
#define DEV_CWR_OK          DEV_COME_OK /*!< No error, OK. */
#define DEV_CWR_NLOCK       DEV_COME_NLOCK /*!< No lock acquired for the device. */
#define DEV_CWR_NOT         DEV_COME_NDEV /*!< Not a character device. */
#define DEV_CWR_BUSY        DEV_COME_BUSY /*!< Device busy */
#define DEV_CWR_OVERFLOW    5 /*!< Buffer overflow */
#define DEV_CWR_INTERNAL    DEV_COME_INTERNAL /*!< Device driver internal error */

/* Character device read error codes */
#define DEV_CRD_OK          DEV_COME_OK /*!< No error, OK. */
#define DEV_CRD_NLOCK       DEV_COME_NLOCK /*!< No lock acquired. */
#define DEV_CRD_NOT         DEV_COME_NDEV /*!< Not a character device. */
#define DEV_CRD_UNDERFLOW   6 /*!< Buffer underflow */
#define DEV_CRD_INTERNAL    DEV_COME_INTERNAL /*!< Device driver internal error */

/* Block device write error codes */
#define DEV_BWR_OK          DEV_COME_OK /*!< No error, OK. */
#define DEV_BWR_NLOCK       DEV_COME_NLOCK /*!< No lock acquired. */
#define DEV_BWR_NOT         DEV_COME_NDEV /*!< Not a block device. */
#define DEV_BWR_BUSY        DEV_COME_BUSY /*!< Device busy */
#define DEV_BWR_EOF         DEV_COME_EOF /*!< End of file */
#define DEV_BWR_INTERNAL    DEV_COME_INTERNAL /*!< Device driver internal error */

/* Block device read error codes */
#define DEV_BRD_OK          DEV_COME_OK /*!< No error, OK. */
#define DEV_BRD_NLOCK       DEV_COME_NLOCK /*!< No lock acquired. */
#define DEV_BRD_NOT         DEV_COME_NDEV /*!< Not a block device. */
#define DEV_BRD_BUSY        DEV_COME_BUSY /*!< Device busy */
#define DEV_BRD_EOF         DEV_COME_EOF /*!< End of file */
#define DEV_BRD_INTERNAL    DEV_COME_INTERNAL /*!< Device driver internal error */

/* Block device seek error codes */
#define DEV_BSK_OK          DEV_COME_OK /*!< No error, OK. */
#define DEV_BSK_NLOCK       DEV_COME_NLOCK /*!< No lock acquired. */
#define DEV_BSK_NOT         DEV_COME_NDEV /*!< Not a block device. */
#define DEV_BSK_BUSY        DEV_COME_BUSY /*!< Device busy */
#define DEV_BSK_EOF         DEV_COME_EOF /*!< End of file */
#define DEV_BSK_INTERNAL    DEV_COME_INTERNAL /*!< Device driver internal error */

/* Block device seek origin types */
#define DEV_BSEEK_SET   0 /*!< Block device seek from beginning. */
#define DEV_BSEEK_CUR   1 /*!< Block device seek from current position */

#endif /* DEVTYPES_H */

/**
  * @}
  */
