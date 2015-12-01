#include <threads.h>
#include <pthread.h>

void mtx_destroy(mtx_t *mtx)
{
    pthread_mutex_destroy(mtx);
}
