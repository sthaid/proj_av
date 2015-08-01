#include <thread>
#include <chrono>

#include "utils.h"

void microsec_sleep(long us)
{
    std::this_thread::sleep_for (std::chrono::microseconds(us));
}

