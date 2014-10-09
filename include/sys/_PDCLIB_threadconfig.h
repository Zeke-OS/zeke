#ifndef _PDCLIB_THREADCONFIG_H
#define _PDCLIB_THREADCONFIG_H
#include <sys/_PDCLIB_aux.h>
#include <sys/_PDCLIB_config.h>

/* Just include pthread.h */
#include <pthread.h>
#define _PDCLIB_THR_T pthread_t
//#define _PDCLIB_CND_T pthread_cond_t
#define _PDCLIB_MTX_T pthread_mutex_t
#define _PDCLIB_TSS_DTOR_ITERATIONS 5
#define _PDCLIB_TSS_T pthread_key_t
typedef pthread_once_t _PDCLIB_once_flag;
#define _PDCLIB_ONCE_FLAG_INIT PTHREAD_ONCE_INIT

_PDCLIB_END_EXTERN_C
#endif
