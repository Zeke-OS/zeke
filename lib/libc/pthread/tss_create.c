#include <threads.h>
#include <pthread.h>

int tss_create(tss_t *key, tss_dtor_t dtor)
{
    switch (pthread_key_create(key, dtor)) {
    case 0:
        return thrd_success;
    default:
        return thrd_error;
    }
}
