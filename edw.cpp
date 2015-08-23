// XXX - add edit pixels

#include <sstream>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "display.h"
#include "world.h"
#include "fixed_control_car.h"
#include "logging.h"
#include "utils.h"

// display and pane size
#define DISPLAY_WIDTH        1420
#define DISPLAY_HEIGHT       800

#define PANE_WORLD_ID        0
#define PANE_WORLD_X         0 
#define PANE_WORLD_Y         0
#define PANE_WORLD_WIDTH     800
#define PANE_WORLD_HEIGHT    800

#define PANE_CTRL_ID         1
#define PANE_CTRL_X          820
#define PANE_CTRL_Y          0
#define PANE_CTRL_WIDTH      600
#define PANE_CTRL_HEIGHT     750

#define PANE_MSG_BOX_ID      2
#define PANE_MSG_BOX_X       820 
#define PANE_MSG_BOX_Y       750
#define PANE_MSG_BOX_WIDTH   600
#define PANE_MSG_BOX_HEIGHT  50 

// world display pane center coordinates and zoom
double       center_x = world::WORLD_WIDTH / 2;
double       center_y = world::WORLD_HEIGHT / 2;
const double ZOOM_FACTOR = 1.1892071;
const double MAX_ZOOM    = 64.0 - .01;
const double MIN_ZOOM    = (1.0 / ZOOM_FACTOR) + .01;
double       zoom = 1.0; 

// simulation cycle time
const int CYCLE_TIME_US = 50000;  // 50 ms

// pane message box 
const int MAX_MESSAGE_TIME_US = 1000000;   
string    message = "";
int       message_time_us = MAX_MESSAGE_TIME_US;

// create roads mode
const int INITIAL_CREATE_ROAD_X = 2048;
const int INITIAL_CREATE_ROAD_Y = 2048;
const int INITIAL_CREATE_ROAD_DIR = 0;
bool      create_roads_run = false;
double    create_road_x = INITIAL_CREATE_ROAD_X;
double    create_road_y = INITIAL_CREATE_ROAD_Y;
double    create_road_dir = INITIAL_CREATE_ROAD_DIR;
int       create_road_steering_idx = 5;

// display message utility
inline void display_message(string msg)
{
    message = msg;
    message_time_us = 0;
}

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    enum mode { MAIN, CREATE_ROADS };

    string    world_filename = "world.dat";
    enum mode mode = MAIN;
    bool      done = false;
    long      start_time_us, end_time_us, delay_us;

    //
    // INITIALIZATION
    //

    // get options
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

    // get args
    if ((argc - optind) >= 1) {
        world_filename = argv[optind];
    }

    // create the display
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // call static initialization routines for the world and car classes
    world::static_init();
    car::static_init(d);

    // create the world
    world w(d,world_filename);
    display_message(w.read_ok() ? "READ SUCCESS" : "READ FAILURE");

    //
    // MAIN LOOP
    //

    while (!done) {
        //
        // STORE THE START TIME
        //

        start_time_us = microsec_timer();

        //
        // DISPLAY UPDATE 
        // 

        // start display update
        d.start(PANE_WORLD_X,   PANE_WORLD_Y,   PANE_WORLD_WIDTH,   PANE_WORLD_HEIGHT, 
                PANE_CTRL_X,    PANE_CTRL_Y,    PANE_CTRL_WIDTH,    PANE_CTRL_HEIGHT,
                PANE_MSG_BOX_X, PANE_MSG_BOX_Y, PANE_MSG_BOX_WIDTH, PANE_MSG_BOX_HEIGHT);

        // draw world 
        w.place_object_init();
        if (mode == CREATE_ROADS) {
            fixed_control_car car(d, w, create_road_x, create_road_y, create_road_dir, 0);
            car.place_car_in_world();
        }
        w.draw(PANE_WORLD_ID,center_x,center_y,zoom);

        // draw for mode CREATE_ROADS
        if (mode == CREATE_ROADS) {
            std::ostringstream s;
            const double distance = 0.5;

            if (create_roads_run) {
                w.create_road_slice(create_road_x, create_road_y, create_road_dir);  

                create_road_y += distance * sin((create_road_dir+270) * (M_PI/180.0));
                create_road_x += distance * cos((create_road_dir+270) * (M_PI/180.0));

                create_road_dir += (create_road_steering_idx - 5) * 0.05;
                if (create_road_dir < 0) {
                    create_road_dir += 360;
                } else if (create_road_dir > 360) {
                    create_road_dir -= 360;
                }
            }

            d.text_draw("^", 1, 2*(create_road_steering_idx-1), 1);

            s.str("");
            if (create_roads_run) {
                int mph = distance / CYCLE_TIME_US * (3600000000.0 / 5280.0);
                s << "RUNNING - " << mph << " MPH";
            } else {
                s << "STOPPED";
            }
            d.text_draw(s.str(), 7, 0, 1);

            s.str("");
            s << "X,Y,DIR: " << static_cast<int>(create_road_x) << " " 
                             << static_cast<int>(create_road_y) << " " 
                             << static_cast<int>(create_road_dir);
            d.text_draw(s.str(), 8, 0, 1);
        }

        // draw the message box
        if (message_time_us < MAX_MESSAGE_TIME_US) {
            d.text_draw(message, 17, 0, PANE_MSG_BOX_ID);
            message_time_us += CYCLE_TIME_US;
        }

        // draw and register events
        int eid_clear=-1, eid_write=-1, eid_reset=-1, eid_quit=-1, eid_quit_win=-1, eid_pan=-1, eid_zoom=-1;
        int eid_create_roads=-1;
        int eid_1=-1, eid_9=-1, eid_run=-1, eid_stop=-1, eid_done=-1, eid_rol=-1, eid_ror=-1, eid_click=-1;
        __attribute__((unused)) int eid_2=-1, eid_3=-1, eid_4=-1, eid_5=-1, eid_6=-1, eid_7=-1, eid_8=-1;

        eid_write    = d.text_draw("WRITE",        13,  0, PANE_CTRL_ID, true);      
        eid_quit     = d.text_draw("QUIT",         13, 10, PANE_CTRL_ID, true);     
        eid_reset    = d.text_draw("RESET",        15,  0, PANE_CTRL_ID, true);     
        eid_clear    = d.text_draw("CLEAR",        15, 10, PANE_CTRL_ID, true);      
        eid_quit_win = d.event_register(display::ET_QUIT);
        eid_pan      = d.event_register(display::ET_MOUSE_MOTION, 0);
        eid_zoom     = d.event_register(display::ET_MOUSE_WHEEL, 0);
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
            eid_rol   = d.text_draw("ROL",           3, 0, 1, true, display::KEY_LEFT);
            eid_ror   = d.text_draw("ROR",           3, 7, 1, true, display::KEY_RIGHT);
            eid_run   = d.text_draw("RUN",           5, 0, 1, true, 'r');      
            eid_stop  = d.text_draw("STOP",          5, 7, 1, true, 's');      
            eid_done  = d.text_draw("DONE",          5,14, 1, true, 'd'); 
            eid_click = d.event_register(display::ET_MOUSE_RIGHT_CLICK, 0);
            break;
        }

        // finish, updates the display
        d.finish();

        //
        // EVENT HADNLING 
        // 

        struct display::event event = d.event_poll();
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
                create_road_x = INITIAL_CREATE_ROAD_X;
                create_road_y = INITIAL_CREATE_ROAD_Y;
                create_road_dir = INITIAL_CREATE_ROAD_DIR;
                break;
            }
            if (event.eid == eid_write) {
                w.write();
                display_message(w.write_ok() ? "WRITE SUCCESS" : "WRITE FAILURE");
                break;
            }
            if (event.eid == eid_reset) {
                w.read();
                display_message(w.read_ok() ? "READ SUCCESS" : "READ FAILURE");
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
                    create_roads_run = true;
                    break;
                }
                if (event.eid == eid_stop) {
                    create_roads_run = false;
                    create_road_dir = (int)(create_road_dir + 0.5);
                    break;
                }
                if (event.eid == eid_done) {
                    create_roads_run = false;
                    mode = MAIN;
                    break;
                }
                if (event.eid >= eid_1 && event.eid <= eid_9) {
                    create_road_steering_idx = event.eid - eid_1 + 1;
                }
                if (event.eid == eid_click) {
                    double world_display_width = world::WORLD_WIDTH / zoom;
                    double world_display_height = world::WORLD_HEIGHT / zoom;
                    create_roads_run = false;
                    create_road_x = (center_x - world_display_width / 2) + 
                                    (world_display_width / PANE_WORLD_WIDTH * event.val1);
                    create_road_y = (center_y - world_display_height / 2) + 
                                    (world_display_height / PANE_WORLD_HEIGHT * event.val2);
                    create_road_dir = 0;
                    // xxx INFO("MOUSE " << event.val1 << " " << event.val2 << endl);
                    // xxx INFO("XY = " << create_road_x << " " << create_road_y << endl);
                }
                if (event.eid == eid_rol) {
                    create_road_dir = (int)(create_road_dir + 0.5) - 1;
                    if (create_road_dir < 0) {
                        create_road_dir += 360;
                    }
                }
                if (event.eid == eid_ror) {
                    create_road_dir = (int)(create_road_dir + 0.5) + 1;
                    if (create_road_dir > 360) {
                        create_road_dir -= 360;
                    }
                }
            }
        } while (0);

        //
        // DELAY TO COMPLETE THE TARGET CYCLE TIME
        //

        // delay to complete CYCLE_TIME_US
        end_time_us = microsec_timer();
        delay_us = CYCLE_TIME_US - (end_time_us - start_time_us);
        microsec_sleep(delay_us);

        // oncer per second, debug print this cycle's processing tie
        static int count;
        if (++count == 1000000 / CYCLE_TIME_US) {
            count = 0;
            INFO("PROCESSING TIME = " << end_time_us-start_time_us << " us" << endl);
        }

#if 1
        // determine average cycle time
        // xxx make this a routine, or just delete
        {
            const int   MAX_TIMES=10;
            static long times[MAX_TIMES];
            double avg_cycle_time = 0;
            memmove(times+1, times, (MAX_TIMES-1)*sizeof(long));
            times[0] = microsec_timer();
            if (times[MAX_TIMES-1] != 0) {
                avg_cycle_time = (times[0] - times[MAX_TIMES-1]) / (MAX_TIMES-1);
            }
            static int count;
            if ((++count % 100) == 0) {
                INFO("AVG CYCLE TIME " << avg_cycle_time / 1000. << endl);
            }
        }
#endif
    }

    return 0;
}

