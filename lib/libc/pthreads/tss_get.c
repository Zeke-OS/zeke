#include <threads.h>
#include <pthread.h>

void *tss_get(tss_t key)
{
    return pthread_getspecific(key);
}
