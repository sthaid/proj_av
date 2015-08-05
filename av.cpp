#include <unistd.h>

#include "display.h"
#include "world.h"
#include "logging.h"
#include "utils.h"

#define PANE_WORLD_WIDTH      800
#define PANE_WORLD_HEIGHT     800
#define PANE_CTRL_WIDTH      600
#define PANE_CTRL_HEIGHT     800
#define PANE_SEPERATION 20

#define DISPLAY_WIDTH   (PANE_WORLD_WIDTH + PANE_SEPERATION + PANE_CTRL_WIDTH)
#define DISPLAY_HEIGHT  (PANE_WORLD_HEIGHT >= PANE_CTRL_HEIGHT ? PANE_WORLD_HEIGHT : PANE_CTRL_HEIGHT)

const double MAX_ZOOM = 31.99;
const double MIN_ZOOM = 0.51;
const double ZOOM_FACTOR = 1.1892071;

double zoom = 0.5;
double center_x = world::WIDTH / 2;
double center_y = world::HEIGHT / 2;

int main_edit(string world_filename);

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    bool opt_edit = false;
    string world_filename;

    // get options
    while (true) {
        char opt_char = getopt(argc, argv, "e");
        if (opt_char == -1) {
            break;
        }
        switch (opt_char) {
        case 'e':
            opt_edit = true;
            break;
        default:
            return 1;
        }
    }

    // get args
    world_filename = "world.dat";
    if ((argc - optind) >= 1) {
        world_filename = argv[optind];
    }

    // handle edit mode0
    if (opt_edit) {
        return main_edit(world_filename);
    }

    // terminate
    return 0;
}

// -----------------  EDIT  ------------------------------------------------------------------------

int main_edit(string world_filename)
{
    enum {MAIN, CREATE_ROADS} edit_ctrls;
    struct display::event event;
    int eid_pan, eid_zoom, eid_quit_win;
    int eid_create_roads, eid_write, eid_reset, eid_quit, eid_force_quit;
    bool done;

    // init
    edit_ctrls = MAIN;
    done = false;

    // create the display
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // create the world
    world w(d,world_filename);
    if (!w.read_ok()) {
        INFO("read " << world_filename << " failed" << endl);
    }

    // loop
    while (true) {
        // start
        d.start(0, 0, PANE_WORLD_WIDTH, PANE_WORLD_HEIGHT,                            // pane 0: x,y,w,h
                PANE_WORLD_WIDTH+PANE_SEPERATION, 0, PANE_CTRL_WIDTH, PANE_CTRL_HEIGHT);  // pane 1: x,y,w,h

        // draw the world
        w.draw(0,center_x,center_y,zoom);

        // register for events
        eid_pan      = d.event_register(display::ET_MOUSE_MOTION, 0);
        eid_zoom     = d.event_register(display::ET_MOUSE_WHEEL, 0);
        eid_quit_win = d.event_register(display::ET_QUIT);
        switch (edit_ctrls) {
        case MAIN:
            eid_create_roads = d.text_draw("CREATE_ROADS",  1, 0, 1, true);  // r,c,pid,event
            eid_write        = d.text_draw("WRITE",        10, 0, 1, true);      
            eid_reset        = d.text_draw("RESET",        11, 0, 1, true);     
            eid_quit         = d.text_draw("QUIT",         12, 0, 1, true);     
            eid_force_quit   = d.text_draw("FORCE_QUIT",   13, 0, 1, true);
            break;
        case CREATE_ROADS:
            break;
        }

        // finish
        d.finish();

        // event handling
        event = d.event_poll();
        if (event.eid != display::EID_NONE) {
            cout << event.eid << endl;
        }
        do {
            // common events
            if (event.eid == display::EID_NONE) {
                break;
            }
            if (event.eid == eid_quit_win) {
                done = true;
                break;
            }
            if (event.eid == eid_pan) {
                center_x -= (double)event.val1 * 8 / zoom;
                center_y -= (double)event.val2 * 8 / zoom;
                break;
            } 
            if (event.eid == eid_zoom) {
                if (event.val2 < 0 && zoom > MIN_ZOOM) {
                    zoom /= ZOOM_FACTOR;
                }
                if (event.val2 > 0 && zoom < MAX_ZOOM) {
                    zoom *= ZOOM_FACTOR;
                }
                break;
            }

            // MAIN edit_ctrls events
            if (edit_ctrls == MAIN) {
                if (event.eid == eid_create_roads) {
                    // edit_ctrls = CREATE_ROADS;
                    break;
                }
                if (event.eid == eid_write) {
                    w.write();
                    break;
                }
                if (event.eid == eid_reset) {
                    w.read();
                    break;
                }
                if (event.eid == eid_quit) {
                    done = true;
                    break;
                }
                if (event.eid == eid_force_quit) {
                    done = true;
                    break;
                }
            }
        } while (0);

        // if done flag set then get out
        if (done) {
            break;
        }

        // XXX edittting

        // delay 
        microsec_sleep(10000);
    }

    return 0;
}










#if 0
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
#endif
