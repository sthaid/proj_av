#include <iostream>
#include <chrono>
#include <thread>
#include "display.h"

//#define DEFAULT_WIDTH   1280  XXX
//#define DEFAULT_HEIGHT  800
#define WIDTH   640
#define HEIGHT  400

void microsec_sleep(long us)
{
    std::this_thread::sleep_for (std::chrono::microseconds(us));
}

int main(int argc, char **argv)
{
    display display(WIDTH,HEIGHT);

    microsec_sleep(1000000);

    return 0;
}
