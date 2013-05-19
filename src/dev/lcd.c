/**
 *******************************************************************************
 * @file    lcd.c
 * @author  Olli Vanhoja
 * @brief   Device driver "wrapper" for LCDs.
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

int lcd_cwrite(uint32_t ch, osDev_t dev);

/**
 * TODO lcd driver should use its own thread to commit slow write operations to
 * keep kernel time small. Also lcd should be set busy while thread is executing.
 */
void lcd_init(int major)
{
    lcdc_init();
    DEV_INIT(major, &lcd_cwrite, 0, 0, 0, 0, 0);
}

/**
 * Write to lcd.
 * TODO
 * - Support size & count?
 * - check minor number
 */
int lcd_cwrite(uint32_t ch, osDev_t dev)
{
    if (!queue_push(&lcdc_queue_cb, &ch)) {
        return DEV_CWR_BUSY;
    }

    return DEV_CWR_OK;
}

/**
  * @}
  */

/**
  * @}
  */
