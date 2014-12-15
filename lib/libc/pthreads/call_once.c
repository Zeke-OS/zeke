#include <threads.h>
#include <pthread.h>

void call_once(once_flag *flag, void (*func)(void))
{
    pthread_once(flag, func);
}
