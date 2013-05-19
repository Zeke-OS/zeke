/**
 *******************************************************************************
 * @file    lcd.c
 * @author  Olli Vanhoja
 * @brief   Device driver for lcd.
 *******************************************************************************
 */

/** @addtogroup Dev
  * @{
  */

/** @addtogroup lcd
  * @{
  */

#include "dev.h"
#include "lcd_ctrl.h"
#include "lcd.h"

int lcd_bwrite(void * buff, size_t size, size_t count, osDev_t dev);

/**
 * TODO lcd driver should use its own thread to commit slow write operations to
 * keep kernel time small. Also lcd should be set busy while thread is executing.
 */
void lcd_init(int major)
{
    lcdc_init();
    DEV_INIT(major, 0, 0, &lcd_bwrite, 0, 0, 0);
}

/**
 * Write to lcd.
 * TODO
 * - Support size & count?
 */
int lcd_bwrite(void * buff, size_t size, size_t count, osDev_t dev)
{
    if (!queue_push(&lcdc_queue_cb, buff)) {
        // error? & set busy
    }
    if (queue_isFull(&lcdc_queue_cb)) {
        // Set dev busy
    }

    return DEV_CWR_OK;
}

/**
  * @}
  */

/**
  * @}
  */
