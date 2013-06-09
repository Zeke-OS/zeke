/**
 *******************************************************************************
 * @file    lcd_ctrl.h
 * @author  Olli Vanhoja
 * @brief   Header for lcd_ctrl.c module.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

/** @addtogroup lcd
  * @{
  */

#pragma once
#ifndef LCD_CTRL_H
#define LCD_CTRL_H

#include "queue.h"

extern queue_cb_t lcdc_queue_cb;

osThreadId lcdc_init(void);

#endif /* LCD_CTRL_H */

/**
  * @}
  */

/**
  * @}
  */
