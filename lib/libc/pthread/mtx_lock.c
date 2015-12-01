#include <threads.h>
#include <pthread.h>

int mtx_lock(mtx_t *mtx)
{
    return (pthread_mutex_lock(mtx)) ? thrd_error : thrd_success;
}
