#include <sstream>
#include <unistd.h>
#include <string.h>

#include "display.h"
#include "world.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;

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

double zoom = 1.0 / ZOOM_FACTOR;
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

// XXX display ptr to current dir rate
// - display speed
// - set cursor, and direction
// - display cursor when in this mode, both in text and on display
//

int main_edit(string world_filename)
{
    enum mode { MAIN, CREATE_ROADS };

    const int MAX_MESSAGE_AGE = 200;
    const int DELAY_MICROSECS = 10000;

    enum mode             mode = MAIN;
    struct display::event event = {display::EID_NONE,0,0};
    bool                  done = false;
    bool                  create_roads_active = false;
    double                create_road_x = 2000;
    double                create_road_y = 2000;
    double                create_road_dir = 0;
    int                   create_road_dir_idx = 5;
    string                message = "";
    int                   message_age = MAX_MESSAGE_AGE;
    double                avg_cycle_time = DELAY_MICROSECS;

    // create the display
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // create the world
    world w(d,world_filename);
    message = w.read_ok() ? "READ SUCCESS" : "READ FAILURE";
    message_age = 0;

    // loop
    while (!done) {
        // determine average cycle time
        {
            const int   MAX_TIMES=10;
            static long times[MAX_TIMES];
            memmove(times+1, times, (MAX_TIMES-1)*sizeof(long));
            times[0] = microsec_timer();
            if (times[MAX_TIMES-1] != 0) {
                avg_cycle_time = (times[0] - times[MAX_TIMES-1]) / (MAX_TIMES-1);
            }
        }
            
        // start
        d.start(0, 0, PANE_WORLD_WIDTH, PANE_WORLD_HEIGHT,                                // pane 0: x,y,w,h
                PANE_WORLD_WIDTH+PANE_SEPERATION, 0, PANE_CTRL_WIDTH, PANE_CTRL_HEIGHT);  // pane 1: x,y,w,h

        // draw the world
        w.draw(0,center_x,center_y,zoom);

        // additional drawing: CREATE_ROADS mode
        if (mode == CREATE_ROADS) {
            ostringstream s;

            // xxx comments
            if (create_roads_active) {
                // xxx dont go off end of world
                w.create_road_slice(create_road_x, create_road_y, create_road_dir);  //xxx return distance

                create_road_dir += (create_road_dir_idx - 5) * 0.05;
                if (create_road_dir < 0) {
                    create_road_dir += 360;
                } else if (create_road_dir > 360) {
                    create_road_dir -= 360;
                }
            }

            d.text_draw("^", 1, 2*(create_road_dir_idx-1), 1);

            s.str("");
            if (create_roads_active) {
                int mph = 0.5 / avg_cycle_time * (3600000000.0 / 5280.0);
                s << "RUNNING - SPEED " << mph;
            } else {
                s << "STOPPED";
            }
            d.text_draw(s.str(), 5, 0, 1);

            s.str("");
            s << "X,Y,DIR: " << static_cast<int>(create_road_x) << " " 
                             << static_cast<int>(create_road_y) << " " 
                             << static_cast<int>(create_road_dir);
            d.text_draw(s.str(), 6, 0, 1);
        }

        // additional drawing: message box 
        if (message_age < MAX_MESSAGE_AGE) {
            d.text_draw(message, 17, 0, 1);
            message_age++;
        }

        // register for events
        int eid_clear=-1, eid_write=-1, eid_reset=-1, eid_quit=-1, eid_quit_win=-1, eid_pan=-1, eid_zoom=-1;
        int eid_create_roads=-1;
        int eid_1=-1, eid_9=-1, eid_run=-1, eid_stop=-1, eid_done=-1;
        __attribute__((unused)) int eid_2=-1, eid_3=-1, eid_4=-1, eid_5=-1, eid_6=-1, eid_7=-1, eid_8=-1;

        eid_write        = d.text_draw("WRITE",        13,  0, 1, true);      
        eid_quit         = d.text_draw("QUIT",         13, 10, 1, true);     
        eid_reset        = d.text_draw("RESET",        15,  0, 1, true);     
        eid_clear        = d.text_draw("CLEAR",        15, 10, 1, true);      
        eid_quit_win     = d.event_register(display::ET_QUIT);
        eid_pan          = d.event_register(display::ET_MOUSE_MOTION, 0);
        eid_zoom         = d.event_register(display::ET_MOUSE_WHEEL, 0);
        switch (mode) {
        case MAIN:
            eid_create_roads = d.text_draw("CREATE_ROADS",  0, 0, 1, true);  // r,c,pid,event
            break;
        case CREATE_ROADS:
            eid_1     = d.text_draw("1",             0, 0, 1, true, '1');
            eid_2     = d.text_draw("2",             0, 2, 1, true, '2');      
            eid_3     = d.text_draw("3",             0, 4, 1, true, '3');      
            eid_4     = d.text_draw("4",             0, 6, 1, true, '4');      
            eid_5     = d.text_draw("5",             0, 8, 1, true, '5');      
            eid_6     = d.text_draw("6",             0,10, 1, true, '6');      
            eid_7     = d.text_draw("7",             0,12, 1, true, '7');      
            eid_8     = d.text_draw("8",             0,14, 1, true, '8');  
            eid_9     = d.text_draw("9",             0,16, 1, true, '9');      
            eid_run   = d.text_draw("RUN",           3, 0, 1, true, 'g');      
            eid_stop  = d.text_draw("STOP",          3, 7, 1, true, 's');      
            eid_done  = d.text_draw("DONE",          3,14, 1, true, 'd'); 
            break;
        }

        // finish/ 
        d.finish();

        // event handling
        event = d.event_poll();
        do {
            // common events
            if (event.eid == display::EID_NONE) {
                break;
            }
            if (event.eid == eid_quit_win || event.eid == eid_quit) {
                done = true;
                break;
            }
            if (event.eid == eid_clear) {
                w.clear();
                break;
            }
            if (event.eid == eid_write) {
                w.write();
                message = w.write_ok() ? "WRITE SUCCESS" : "WRITE FAILURE";
                message_age = 0;
                break;
            }
            if (event.eid == eid_reset) {
                w.read();
                message = w.read_ok() ? "READ SUCCESS" : "READ FAILURE";
                message_age = 0;
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

            // MAIN mode events
            if (mode == MAIN) {
                if (event.eid == eid_create_roads) {
                    mode = CREATE_ROADS;
                    break;
                }
            }

            // CREATE_ROADS mode events
            if (mode == CREATE_ROADS) {
                if (event.eid == eid_run) {
                    create_roads_active = true;
                    break;
                }
                if (event.eid == eid_stop) {
                    create_roads_active = false;
                    break;
                }
                if (event.eid == eid_done) {
                    create_roads_active = false;
                    mode = MAIN;
                    break;
                }
                if (event.eid >= eid_1 && event.eid <= eid_9) {
                    create_road_dir_idx = event.eid - eid_1 + 1;
                }
            }
        } while (0);

        // delay 
        microsec_sleep(DELAY_MICROSECS);
    }

    return 0;
}
