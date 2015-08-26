#include <thread>
#include <chrono>
#include <time.h>

#include "utils.h"

void microsec_sleep(long us)
{
    if (us <= 0) {
        return;
    }
    std::this_thread::sleep_for (std::chrono::microseconds(us));
}

long microsec_timer(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC,&ts);
    return  ((long)ts.tv_sec * 1000000) + ((long)ts.tv_nsec / 1000);
}


