#include <threads.h>
#include <pthread.h>

static int count = 0;
static once_flag once = ONCE_FLAG_INIT;

static void do_once(void)
{
    count++;
}

int main(void)
{
    TESTCASE(count == 0);
    call_once(&once, do_once);
    TESTCASE(count == 1);
    call_once(&once, do_once);
    TESTCASE(count == 1);
    do_once();
    TESTCASE(count == 2);

    return TEST_RESULTS;
}
