#include "display.h"
#include "world.h"
#include "logging.h"
#include "utils.h"

#define GRID_WIDTH      800
#define GRID_HEIGHT     800
#define CTRL_WIDTH      600
#define CTRL_HEIGHT     800
#define PANE_SEPERATION 20

#define DISPLAY_WIDTH   (GRID_WIDTH + PANE_SEPERATION + CTRL_WIDTH)
#define DISPLAY_HEIGHT  (GRID_HEIGHT >= CTRL_HEIGHT ? GRID_HEIGHT : CTRL_HEIGHT)

const double MAX_ZOOM = 31.99;
const double MIN_ZOOM = 0.51;
const double ZOOM_FACTOR = 1.1892071;

double zoom = 0.5;
double center_x = world::WIDTH / 2;
double center_y = world::HEIGHT / 2;
bool   create_roads = false;

// XXX
// pgm args : //  -e world.dat
// controls for road creation
//    - start location, and direction
//    - steering
// rest the world
// force quit to discard changes   (WRITE, QUIT, FORCE_QUIT

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    // create the display
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // create the world
    world w(d,"world.dat");
    if (!w.read_ok()) {
        WARNING("read world.dat failed" << endl);
    }

    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 100; j++) {
            w.set_location(i,j,display::BLUE);
        }
    }

    // loop
    while (true) {
        // start
        d.start(0, 0, GRID_WIDTH, GRID_HEIGHT,                            // pane 0: x,y,w,h
                GRID_WIDTH+PANE_SEPERATION, 0, CTRL_WIDTH, CTRL_HEIGHT);  // pane 1: x,y,w,h

        // draw the world
        w.draw(0,center_x,center_y,zoom);

        // register for events
        int eid_quit      = d.event_register(display::ET_QUIT);
        int eid_write     = d.text_draw("WRITE",    1, 0, 1, true);      // r,c,pid,event
        int eid_pan       = d.event_register(display::ET_MOUSE_MOTION, 0);
        int eid_zoom      = d.event_register(display::ET_MOUSE_WHEEL, 0);
        int eid_create_roads = d.text_draw("CREATE_ROADS",  3, 0, 1, true);

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
            center_x -= (double)event.val1 * 8 / zoom;
            center_y -= (double)event.val2 * 8 / zoom;
        } else if (event.eid == eid_zoom) {
            if (event.val2 < 0 && zoom > MIN_ZOOM) {
                zoom /= ZOOM_FACTOR;
            }
            if (event.val2 > 0 && zoom < MAX_ZOOM) {
                zoom *= ZOOM_FACTOR;
            }
        } else if (event.eid == eid_create_roads) {
            INFO("CREATE_ROADS starting " << endl);
            create_roads = true;
        } else if (event.eid == display::EID_NONE) {
            ; 
        } else {
            FATAL("invalid eid " << event.eid << endl);
        }

        // if draw mode is enabled then draw roads
        if (create_roads) {
            static int count = 0;
            static double x = 0, y = 2000, dir = 0;

            w.create_road_slice(x,y,dir);

            dir += .1;

            if (count++ > 4000) {
                INFO("CREATE_ROADS complete " << endl);
                create_roads = false;
            }
        }

        // delay 
        microsec_sleep(10000);

        //static int count;
        //INFO("count" << count++ << endl);
    }

    // exit
    return 0;
}
