#include <iostream>
#include <chrono>
#include <thread>
#include <memory.h>

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
    int evid_line2, evid_quit, evid_keybd, evid_winsz;

    display d(WIDTH,HEIGHT);

    INFO("width,height,minimized = " << d.get_win_width() << d.get_win_height() << d.get_win_minimized() << endl);

    // microsec_sleep(10000000);  // XXX
    // return 0;

    unsigned char * pixels = new unsigned char[1000000];  // XXX or use uint8
    memset(pixels,0,1000000);
    memset(pixels+500000,4,10000);

    struct display::image * img = d.create_image(pixels, 1000, 1000);   
    cout << "image " << img << endl;

    while (true) {
        d.start(540,300,100,100,   // pane 1
                100,100,200,200);  // pane 2

        d.set_color(display::PURPLE);
        d.draw_point(50, 50, 1);
        d.draw_rect(0, 0, 100, 100, 1);
        d.draw_text("hello", 0, 0, 1);

        evid_line2 = d.draw_text("line2", 1, 0, 1, true);
        evid_quit  = d.event_register(display::ET_QUIT);
        evid_keybd = d.event_register(display::ET_KEYBOARD);
        evid_winsz = d.event_register(display::ET_WIN_SIZE_CHANGE);

        d.draw_image(img, 2);

        d.finish();

        struct display::event event = d.poll_event();
        if (event.eid == evid_line2) {
            cout << "GOT line2" << endl;
        } else if (event.eid == evid_quit) {
            cout << "GOT quit" << endl;
            break;
        } else if (event.eid == evid_keybd) {
            cout << "GOT keybd  " << event.val1 << endl;
        } else if (event.eid == evid_winsz) {
            cout << "GOT winsz " << event.val1 << " " << event.val2 << endl;
        } else if (event.eid == display::EID_NONE) {
            ;
        } else {
            cout << "GOT unexpected" << event.eid << endl;
            break;
        }
            
        microsec_sleep(10000);
    }

    return 0;
}
