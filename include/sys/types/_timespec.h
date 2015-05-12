#ifndef TIMESPEC_H
#define TIMESPEC_H

#include <sys/types/_time_t.h>

struct timespec {
    time_t tv_sec;  /*!< Seconds. */
    long tv_nsec;   /*!< Nanoseconds. */
};

#endif
