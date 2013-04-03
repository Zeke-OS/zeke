/**
 *******************************************************************************
 * @file    devnull.h
 * @author  Olli Vanhoja
 * @brief   Device driver for dev null.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

/** @addtogroup null
  * @{
  */

#pragma once
#ifndef DEVNULL_H
#define DEVNULL_H

#include "dev.h"

int devnull_cwrite(uint32_t ch, dev_t dev);
int devnull_cread(uint32_t * ch, dev_t dev);

devnull_init(int major)
{
    DEV_INIT(major, &devnull_cwrite, &devnull_cread, 0, 0, 0);
}

int devnull_cwrite(uint32_t ch, dev_t dev)
{
    return DEV_CWR_OK;
}

int devnull_cread(uint32_t * ch, dev_t dev)
{
    return DEV_CRD_UNDERFLOW;
}

#endif /* DEVNULL_H */

/**
  * @}
  */

/**
  * @}
  */

