#include <threads.h>
#include <pthread.h>
#include <assert.h>

void tss_delete(tss_t key)
{
    assert(pthread_key_delete(key) == 0);
}
