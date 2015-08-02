#include "display.h"
#include "world.h"
#include "logging.h"
#include "utils.h"

#define DISPLAY_WIDTH   (512 + 38 + 250)
#define DISPLAY_HEIGHT  (512)

const double MAX_ZOOM = 31.99;
const double MIN_ZOOM = 0.51;
const double ZOOM_FACTOR = 1.1892071;

double zoom = 0.5;
double center_x = world::WIDTH / 2;
double center_y = world::HEIGHT / 2;

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    // create the world
    // XXX check
    world w("world.dat");

    // create the display
    // XXX option for resizing?
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // loop
    while (true) {
        // start
        d.start(0, 0, 512, 512,     // pane 0
                550, 0, 250, 512);  // pane 1

        // draw the world
        w.draw(d,0,center_x,center_y,zoom);

        // register for events
        int eid_quit      = d.event_register(display::ET_QUIT);
        int eid_write     = d.draw_text("WRITE",    1, 0, 1, true);      // r,c,pid,event
        int eid_pan       = d.event_register(display::ET_MOUSE_MOTION, 0);
        int eid_zoom      = d.event_register(display::ET_MOUSE_WHEEL, 0);
        int eid_draw_road = d.draw_text("DRAW_RD",  3, 0, 1, true);

        // finish
        d.finish();

        // handle events
        struct display::event event = d.poll_event();
        if (event.eid == display::EID_NONE) {
            ;
        } else if (event.eid == eid_quit) {
            break;
        } else if (event.eid == eid_write) {
            w.write("world.dat");
        } else if (event.eid == eid_pan) {
            center_x -= (double)event.val1 * 16 / zoom;
            center_y -= (double)event.val2 * 16 / zoom;
        } else if (event.eid == eid_zoom) {
            if (event.val2 > 0 && zoom > MIN_ZOOM) {
                zoom /= ZOOM_FACTOR;
            }
            if (event.val2 < 0 && zoom < MAX_ZOOM) {
                zoom *= ZOOM_FACTOR;
            }
        } else if (event.eid == eid_draw_road) {
            INFO("DRAW " << endl);
        } else if (event.eid == display::EID_NONE) {
            ; 
        } else {
            FATAL("invalid eid " << event.eid << endl);
        }

        // delay 
        microsec_sleep(10000);
    }

    // exit
    return 0;
}
