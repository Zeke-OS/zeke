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

void devnull_init(int major);
int devnull_cwrite(uint32_t ch, osDev_t dev);
int devnull_cread(uint32_t * ch, osDev_t dev);

#endif /* DEVNULL_H */

/**
  * @}
  */

/**
  * @}
  */

