#include <iostream>
#include <chrono>
#include <thread>

#include "display.h"
#include "logging.h"

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
    display d(WIDTH,HEIGHT);

    INFO("width,height,minimized = " << d.get_win_width() << d.get_win_height() << d.get_win_minimized() << endl);
    d.start();
    d.set_color(display::WHITE);
    d.draw_point(0, 320, 200);
    d.finish();

    microsec_sleep(3000000);

    return 0;
}
