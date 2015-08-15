#include <sstream>
#include <unistd.h>
#include <string.h>

#include "display.h"
#include "world.h"
#include "fixed_control_car.h"
#include "logging.h"
#include "utils.h"

// XXX panes for car display   1,2
#define PANE_WORLD_WIDTH     800
#define PANE_WORLD_HEIGHT    800
#define PANE_CTRL_WIDTH      600
#define PANE_CTRL_HEIGHT     800
#define PANE_SEPERATION      20

#define DISPLAY_WIDTH   (PANE_WORLD_WIDTH + PANE_SEPERATION + PANE_CTRL_WIDTH)
#define DISPLAY_HEIGHT  (PANE_WORLD_HEIGHT >= PANE_CTRL_HEIGHT ? PANE_WORLD_HEIGHT : PANE_CTRL_HEIGHT)

const double ZOOM_FACTOR = 1.1892071;
const double MAX_ZOOM    = 64.0 - .01;
const double MIN_ZOOM    = (1.0 / ZOOM_FACTOR) + .01;

double zoom = 1.0 / ZOOM_FACTOR;
double center_x = world::WORLD_WIDTH / 2;
double center_y = world::WORLD_HEIGHT / 2;

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    typedef class fixed_control_car CAR;

    const int    MAX_CAR = 100;
    const int    MAX_MESSAGE_AGE = 200;
    const int    DELAY_MICROSECS = 10000;

    enum mode { RUN, PAUSE };

    string       world_filename = "world.dat";
    enum mode    mode = PAUSE;
    string       message = "";
    int          message_age = MAX_MESSAGE_AGE;
    bool         done = false;
    int          max_car = 0;
    CAR        * car[MAX_CAR];

    //
    // INITIALIZATION
    //

    // get options, and args
    while (true) {
        char opt_char = getopt(argc, argv, "");
        if (opt_char == -1) {
            break;
        }
        switch (opt_char) {
        default:
            return 1;
        }
    }
    if ((argc - optind) >= 1) {
        world_filename = argv[optind];
    }

    // create the display
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // create the world
    world w(d,world_filename);
    message = w.read_ok() ? "READ SUCCESS" : "READ FAILURE";
    message_age = 0;

    // create cars
#if 1
    for (double dir = 0; dir < 360; dir += 10) {
        car[max_car++] = new CAR(d,w,2048,2048,dir, 10);
    }
#else
    car[max_car++] = new CAR(d,w,2048,2048,0,0);
#endif

    //
    // MAIN LOOP
    //

    while (!done) {
        //
        // CAR SIMULATION
        // 

        // update all car mechanics: position, direction, speed
        if (mode == RUN) {            
            for (int i = 0; i < max_car; i++) {
                car[i]->update_mechanics(DELAY_MICROSECS);
            }
        }

        // update car positions in the world 
        w.place_car_init();
        for (int i = 0; i < max_car; i++) {
            w.place_car(car[i]->get_x(), car[i]->get_y(), car[i]->get_dir());
        }

        // update all car controls: steering and speed
        if (mode == RUN) {            
            for (int i = 0; i < max_car; i++) {
                car[i]->update_controls(DELAY_MICROSECS);
            }
        }

        //
        // DISPLAY UPDATE 
        // 

        // start display update
        d.start(0, 0, PANE_WORLD_WIDTH, PANE_WORLD_HEIGHT,                                // pane 0: x,y,w,h
                PANE_WORLD_WIDTH+PANE_SEPERATION, 0, PANE_CTRL_WIDTH, PANE_CTRL_HEIGHT);  // pane 1: x,y,w,h

        // draw world 
        w.draw(0,center_x,center_y,zoom);

        // draw car state
        car[0]->draw(1,2);

        // draw the message box
        if (message_age < MAX_MESSAGE_AGE) {
            d.text_draw(message, 17, 0, 1);
            message_age++;
        }

        // draw and register events
        int eid_quit_win = d.event_register(display::ET_QUIT);
        int eid_pan      = d.event_register(display::ET_MOUSE_MOTION, 0);
        int eid_zoom     = d.event_register(display::ET_MOUSE_WHEEL, 0);
        int eid_run      = d.text_draw("RUN",           5, 0, 1, true, 'r');      
        int eid_pause    = d.text_draw("PAUSE",         5, 7, 1, true, 's');      

        // finish, updates the display
        d.finish();

        //
        // EVENT HADNLING 
        // 

        struct display::event event = d.event_poll();
        do {
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
            if (event.eid == eid_run) {
                mode = RUN;
                break;
            }
            if (event.eid == eid_pause) {
                mode = PAUSE;
                break;
            }
        } while(0);

        //
        // DELAY
        // 

        microsec_sleep(DELAY_MICROSECS);

        // determine average cycle time
        // XXX make this a routine
        {
            const int   MAX_TIMES=10;
            static long times[MAX_TIMES];
            int avg_cycle_time = 0;
            memmove(times+1, times, (MAX_TIMES-1)*sizeof(long));
            times[0] = microsec_timer();
            if (times[MAX_TIMES-1] != 0) {
                avg_cycle_time = (times[0] - times[MAX_TIMES-1]) / (MAX_TIMES-1);
            }

            // xxx temp
            static int count;
            if ((++count % 100) == 0) {
                INFO("AVG CYCLE TIME " << avg_cycle_time / 1000. << endl);
            }
        }
    }

    return 0;
}
