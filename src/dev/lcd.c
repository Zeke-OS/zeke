/**
 *******************************************************************************
 * @file    lcd.c
 * @author  Olli Vanhoja
 * @brief   Device driver "wrapper" for LCDs.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
