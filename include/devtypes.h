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

typedef uint32_t dev_t; /*!< Device identifier */

#define DEV_MINORBITS   27
#define DEV_MINORMASK   ((1u << MINORBITS) - 1)

/**
 * Get major number from dev_t.
 */
#define DEV_MAJOR(dev)  ((unsigned int)((dev) >> MINORBITS))

/**
 * Get minor number from dev_t.
 */
#define DEV_MINOR(dev)  ((unsigned int)((dev) & MINORMASK))

/**
 * Convert major, minor pair into dev_t.
 */
#define DEV_MMTODEV(ma, mi) (((ma) << MINORBITS) | (mi))


/* Device lock & open error codes */
#define DEV_OERR_OK         0x0
#define DEV_OERR_LOCKED     0x2  /*!< Device is locked by another thread. */
#define DEV_OERR_NONLOCK    0x4  /*!< Device is non-lockable. */
#define DEV_OERR_INTERNAL   0x8  /*!< Device driver internal error. */
#define DEV_OERR_UNKNOWN    0x10 /*!< Unknown device/No driver for this device. */

/* Device close error codes */
#define DEV_CERR_OK         0
#define DEV_CERR_NOT        1 /*!< Device not reserved for this thread. */
#define DEV_CERR_LOCK       2 /*!< Unable to unlock/Device driver is reserved. */

/* Character device write error codes */
#define DEV_CWR_OK          0
#define DEV_CWR_NLOCK       1 /*!< No lock acquired. */
#define DEV_CWR_NOT         2 /*!< Not a character device. */
#define DEV_CWR_BUSY        3 /*!< Device busy */
#define DEV_CWR_OVERFLOW    4 /*!< Buffer overflow */

/* Character device read error codes */
#define DEV_CRD_OK          0
#define DEV_CWR_NLOCK       1 /*!< No lock acquired. */
#define DEV_CRD_NOT         2 /*!< Not a character device. */
#define DEV_CRD_UNDERFLOW   3 /*!< Buffer underflow */

/* Block device write error codes */
#define DEV_BWR_OK          0
#define DEV_BWR_NLOCK       1 /*!< No lock acquired. */
#define DEV_BWR_NOT         1 /*!< Not a block device. */
#define DEV_BWR_BUSY        2 /*!< Device busy */

/* Block device read error codes */
#define DEV_BRD_OK          0
#define DEV_BRD_NLOCK       1 /*!< No lock acquired. */
#define DEV_BRD_NOT         1 /*!< Not a block device. */
#define DEV_BRD_BUSY        2 /*!< Device busy */

#endif /* DEVTYPES_H */

/**
  * @}
  */
