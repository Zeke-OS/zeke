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
#include "sched.h"
#include "ksignal.h"
#include "lcd_ctrl.h"
#include "lcd.h"

int lcd_cwrite(uint32_t ch, osDev_t dev);
static osThreadId lcdc_thread_id;

/**
 * TODO lcd driver should use its own thread to commit slow write operations to
 * keep kernel time small. Also lcd should be set busy while thread is executing.
 */
void lcd_init(int major)
{
    lcdc_thread_id = lcdc_init();
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
    threadInfo_t * lcdc_thread = sched_get_pThreadInfo(lcdc_thread_id);

    /* TODO There migh be some nasty bug somewhere because I had to use this
     * little work-around to get things working properly... :/ */
    lcdc_thread->flags &= ~SCHED_NO_SIG_FLAG;
    lcdc_thread->sig_wait_mask = 1;
    /* End of kludge */
    ksignal_threadSignalSet(lcdc_thread_id, 1);

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
