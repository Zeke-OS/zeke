/**
 *******************************************************************************
 * @file    devnull.c
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

void devnull_init(int major)
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

/**
  * @}
  */

/**
  * @}
  */