#include <threads.h>
#include <pthread.h>

int mtx_unlock(mtx_t *mtx)
{
    if(pthread_mutex_unlock(mtx) == 0)
        return thrd_success;
    return thrd_error;
}
