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
#ifndef LCD_H
#define LCD_H

void lcdc_init(void);
void lcdc_write(const char * c);
void lcdc_data_write(uint8_t data);
void lcdc_reg_write(uint8_t val);
void lcdc_clear(void);
void lcdc_home(void);
void lcdc_goto(char pos);
void lcdc_print(char pos, const char * str);

#endif /* LCD_H */


/**
  * @}
  */

/**
  * @}
  */