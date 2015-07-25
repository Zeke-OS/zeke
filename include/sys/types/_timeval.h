#ifndef TIMEVAL_DECLARED
#define TIMEVAL_DECLARED
#include <sys/types/_time_t.h>
#include <sys/types/_suseconds_t.h>

/**
 * timeval.
 */
struct timeval {
    time_t tv_sec;          /*!< Seconds. */
    suseconds_t tv_usec;    /*!< Microseconds. */
};
#endif
