#include <threads.h>
#include <pthread.h>

int mtx_lock(mtx_t *mtx)
{
    if(pthread_mutex_lock(mtx))
        return thrd_error;
    else return thrd_success;
}
