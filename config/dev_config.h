#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H
/* Include device drivers here */
#include "../src/dev/devnull.h"
#include "../src/dev/lcd.h"
#else /* Declare device drivers here with DEV_DECLARE(major, dev) */
/* TODO something easier, please */
#if configDEVSUBSYS_NULL != 0
DEV_DECLARE(0, devnull)
#endif
#if configDEVSUBSYS_LCD != 0
DEV_DECLARE(1, lcd)
#endif
#endif
