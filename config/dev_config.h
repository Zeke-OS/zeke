#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H
/* Include device drivers here */
#include "../src/dev/devnull.h"
#include "../src/dev/lcd.h"
#else /* Declare device drivers here with DEV_DECLARE(major, dev) */
DEV_DECLARE(1, devnull)
DEV_DECLARE(0, lcd)
#endif
