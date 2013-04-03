#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H
/* Include device drivers here */
#include "devnull.h"
#else
/* Declare device drivers here */
DEV_DECLARE(0, devnull)
#endif
