/*
Copyright (c) 2015 Steven Haid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <sstream>
#include <cmath>  

#include <unistd.h>  // for getopt

#include "display.h"
#include "world.h"
#include "car.h"
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
const double MAX_ZOOM    = 256.0 - .01;
const double MIN_ZOOM    = (1.0 / ZOOM_FACTOR) + .01;
double       zoom = 1.0; 

// simulation cycle time
const int CREATE_ROADS_CYCLE_TIME_US = 25000;

// pane message box 
const int MAX_MESSAGE_TIME_US = 1000000;   
string    message = "";
long      message_start_time_us;

// create roads mode
const int INITIAL_CREATE_ROAD_X = 2048;
const int INITIAL_CREATE_ROAD_Y = 2048;
const int INITIAL_CREATE_ROAD_DIR = 0;
bool      create_roads_run = false;
double    create_road_x = INITIAL_CREATE_ROAD_X;
double    create_road_y = INITIAL_CREATE_ROAD_Y;
double    create_road_dir = INITIAL_CREATE_ROAD_DIR;
int       create_road_steering_idx = 5;
void create_road_slice(class world &w, double x, double y, double dir);

// edit pixels mode
typedef struct {
    int x,y,w,h;
    enum display::color color;
} edit_pixels_color_select_t;
const int MAX_EDIT_PIXELS_COLOR_SELECT = 4;
edit_pixels_color_select_t edit_pixels_color_select_tbl[MAX_EDIT_PIXELS_COLOR_SELECT] = 
    { { 0,    50, 75, 75, display::GREEN  },
      { 150,  50, 75, 75, display::BLACK  },
      { 300,  50, 75, 75, display::YELLOW },
      { 450,  50, 75, 75, display::RED    },
          };
display::color edit_pixels_color_selection = display::GREEN;
void edit_pixels(class world &w, bool start, int x, int y);

// display message utility
inline void display_message(string msg)
{
    message = msg;
    message_start_time_us = microsec_timer();
}

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
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
    string filename = "world.dat";
    if ((argc - optind) >= 1) {
        filename = argv[optind];
    }

    // create the display
    display d(DISPLAY_WIDTH, DISPLAY_HEIGHT);

    // call static initialization routines for the world and car classes
    world::static_init();
    car::static_init(d);

    // create the world
    world w(d);
    bool success = w.read(filename);
    display_message(success ? "READ SUCCESS" : "READ FAILURE");

    //
    // MAIN LOOP
    //

    enum mode { MAIN, CREATE_ROADS, EDIT_PIXELS };
    enum mode mode = MAIN;
    bool      done = false;

    while (!done) {
        //
        // STORE THE START TIME
        //

        long start_time_us = microsec_timer();

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
            car car(d, w, 0, create_road_x, create_road_y, create_road_dir, 0, 50);
            car.place_car_in_world();
        }
        w.draw(PANE_WORLD_ID,center_x,center_y,zoom);

        // draw for mode CREATE_ROADS
        if (mode == CREATE_ROADS) {
            std::ostringstream s;
            const double distance = 0.5;

            if (create_roads_run) {
                create_road_slice(w, create_road_x, create_road_y, create_road_dir);  

                create_road_y -= distance * cos((create_road_dir) * (M_PI/180.0));
                create_road_x += distance * sin((create_road_dir) * (M_PI/180.0));

                create_road_dir = sanitize_direction(create_road_dir + 
                                                     0.05 * (create_road_steering_idx - 5));
            }

            d.text_draw("^", 1, 2*(create_road_steering_idx-1), 1);

            s.str("");
            if (create_roads_run) {
                int mph = distance / CREATE_ROADS_CYCLE_TIME_US * (3600000000.0 / 5280.0);
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

        // draw for mode EDIT_PIXELS
        if (mode == EDIT_PIXELS) {
            for (int i = 0; i < MAX_EDIT_PIXELS_COLOR_SELECT; i++) {
                edit_pixels_color_select_t &p = edit_pixels_color_select_tbl[i];
                d.draw_set_color(display::WHITE);
                d.draw_rect(p.x-2, p.y-2, p.w+4, p.h+4, PANE_CTRL_ID, 2);
                d.draw_set_color(p.color);
                d.draw_filled_rect(p.x, p.y, p.w, p.h, PANE_CTRL_ID);
            }

            d.draw_set_color(edit_pixels_color_selection);
            d.draw_filled_rect(150, 300, 300, 75, PANE_CTRL_ID);
            d.draw_set_color(display::WHITE);
            d.draw_rect(150-2, 300-2, 300+4, 75+4,  PANE_CTRL_ID, 2);
        }

        // draw the message box
        if (microsec_timer() - message_start_time_us < MAX_MESSAGE_TIME_US) {
            d.text_draw(message, 0, 0, PANE_MSG_BOX_ID);
        }

        // draw and register events
        int eid_clear=-1, eid_write=-1, eid_reset=-1, eid_quit=-1, eid_quit_win=-1, eid_pan=-1, eid_zoom=-1;
        int eid_create_roads=-1, eid_edit_pixels=-1;

        int eid_1=-1, eid_9=-1, eid_run=-1, eid_stop=-1, eid_back=-1, eid_rol=-1, eid_ror=-1, eid_cr_click=-1;
        __attribute__((unused)) int eid_2=-1, eid_3=-1, eid_4=-1, eid_5=-1, eid_6=-1, eid_7=-1, eid_8=-1;

        int eid_color_select[MAX_EDIT_PIXELS_COLOR_SELECT];
        for (int i = 0; i < MAX_EDIT_PIXELS_COLOR_SELECT; i++) {
            eid_color_select[i] = -1;
        }
        int eid_ep_motion = -1;

        eid_write    = d.text_draw("WRITE",        13,  0, PANE_CTRL_ID, true);      
        eid_quit     = d.text_draw("QUIT",         13,  8, PANE_CTRL_ID, true);     
        eid_reset    = d.text_draw("RESET",        15,  0, PANE_CTRL_ID, true);     
        eid_clear    = d.text_draw("CLEAR",        15,  8, PANE_CTRL_ID, true);      
        eid_quit_win = d.event_register(display::ET_QUIT);
        eid_pan      = d.event_register(display::ET_MOUSE_LEFT_MOTION, PANE_WORLD_ID);
        eid_zoom     = d.event_register(display::ET_MOUSE_WHEEL, PANE_WORLD_ID);
        switch (mode) {
        case MAIN:
            eid_create_roads = d.text_draw("CREATE_ROADS",  0, 0, PANE_CTRL_ID, true, 'C');
            eid_edit_pixels  = d.text_draw("EDIT_PIXELS",   1, 0, PANE_CTRL_ID, true, 'E');
            break;
        case CREATE_ROADS:
            eid_1        = d.text_draw("1",             0, 0, PANE_CTRL_ID, true, '1');
            eid_2        = d.text_draw("2",             0, 2, PANE_CTRL_ID, true, '2');      
            eid_3        = d.text_draw("3",             0, 4, PANE_CTRL_ID, true, '3');      
            eid_4        = d.text_draw("4",             0, 6, PANE_CTRL_ID, true, '4');      
            eid_5        = d.text_draw("5",             0, 8, PANE_CTRL_ID, true, '5');      
            eid_6        = d.text_draw("6",             0,10, PANE_CTRL_ID, true, '6');      
            eid_7        = d.text_draw("7",             0,12, PANE_CTRL_ID, true, '7');      
            eid_8        = d.text_draw("8",             0,14, PANE_CTRL_ID, true, '8');  
            eid_9        = d.text_draw("9",             0,16, PANE_CTRL_ID, true, '9');      
            eid_rol      = d.text_draw("ROL",           3, 0, PANE_CTRL_ID, true, display::KEY_LEFT);
            eid_ror      = d.text_draw("ROR",           3, 8, PANE_CTRL_ID, true, display::KEY_RIGHT);
            eid_run      = d.text_draw("RUN",           5, 0, PANE_CTRL_ID, true, 'R');      
            eid_stop     = d.text_draw("STOP",          5, 8, PANE_CTRL_ID, true, 'S');      
            eid_back     = d.text_draw("BACK",         13,16, PANE_CTRL_ID, true, 'B'); 
            eid_cr_click = d.event_register(display::ET_MOUSE_RIGHT_CLICK, PANE_WORLD_ID);
            break;
        case EDIT_PIXELS:
            eid_back  = d.text_draw("BACK",         13,16, PANE_CTRL_ID, true, 'B'); 
            eid_ep_motion = d.event_register(display::ET_MOUSE_RIGHT_MOTION, PANE_WORLD_ID);
            for (int i = 0; i < MAX_EDIT_PIXELS_COLOR_SELECT; i++) {
                edit_pixels_color_select_t &p = edit_pixels_color_select_tbl[i];
                eid_color_select[i] = d.event_register(display::ET_MOUSE_LEFT_CLICK, PANE_CTRL_ID, p.x, p.y, p.w, p.h);
            }
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
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_clear) {
                w.clear();
                create_road_x = INITIAL_CREATE_ROAD_X;
                create_road_y = INITIAL_CREATE_ROAD_Y;
                create_road_dir = INITIAL_CREATE_ROAD_DIR;
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_write) {
                bool success = w.write(filename);
                display_message(success ? "WRITE SUCCESS" : "WRITE FAILURE");
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_reset) {
                bool success = w.read(filename);
                display_message(success ? "READ SUCCESS" : "READ FAILURE");
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_pan) {
                center_x -= (double)event.motion.delta_x * 8 / zoom;
                center_y -= (double)event.motion.delta_y * 8 / zoom;
                break;
            } 
            if (event.eid == eid_zoom) {
                if (event.wheel.delta_y < 0 && zoom > MIN_ZOOM) {
                    zoom /= ZOOM_FACTOR;
                }
                if (event.wheel.delta_y > 0 && zoom < MAX_ZOOM) {
                    zoom *= ZOOM_FACTOR;
                }
                break;
            }

            // MAIN mode events
            if (mode == MAIN) {
                if (event.eid == eid_create_roads) {
                    mode = CREATE_ROADS;
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_edit_pixels) {
                    mode = EDIT_PIXELS;
                    d.event_play_sound();
                    break;
                }
            }

            // CREATE_ROADS mode events
            if (mode == CREATE_ROADS) {
                if (event.eid == eid_run) {
                    create_roads_run = true;
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_stop) {
                    create_roads_run = false;
                    create_road_dir = (int)(create_road_dir + 0.5);
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_back) {
                    create_roads_run = false;
                    mode = MAIN;
                    d.event_play_sound();
                    break;
                }
                if (event.eid >= eid_1 && event.eid <= eid_9) {
                    create_road_steering_idx = event.eid - eid_1 + 1;
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_cr_click) {
                    int x,y;
                    w.cvt_coord_pixel_to_world((double)event.click.x/PANE_WORLD_WIDTH, 
                                               (double)event.click.y/PANE_WORLD_HEIGHT, 
                                               x, y);
                    create_road_x = x;
                    create_road_y = y;
                    create_road_dir = 0;
                    create_roads_run = false;
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_rol) {
                    create_road_dir = (int)(create_road_dir + 0.5) - 1;
                    create_road_dir = sanitize_direction(create_road_dir);
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_ror) {
                    create_road_dir = (int)(create_road_dir + 0.5) + 1;
                    create_road_dir = sanitize_direction(create_road_dir);
                    d.event_play_sound();
                    break;
                }
            }

            // EDIT_PIXELS mode events
            if (mode == EDIT_PIXELS) {
                if (event.eid == eid_back) {
                    create_roads_run = false;
                    mode = MAIN;
                    d.event_play_sound();
                    break;
                }
                if (event.eid == eid_ep_motion) {
                    int x,y;
                    bool start = (event.motion.state == display::MOTION_START);
                    w.cvt_coord_pixel_to_world((double)event.motion.x/PANE_WORLD_WIDTH, 
                                               (double)event.motion.y/PANE_WORLD_HEIGHT, 
                                               x, y);
                    edit_pixels(w, start, x, y);
                    break;
                }
                if (event.eid >= eid_color_select[0] && event.eid <= eid_color_select[MAX_EDIT_PIXELS_COLOR_SELECT-1]) {
                    edit_pixels_color_selection = 
                            edit_pixels_color_select_tbl[event.eid - eid_color_select[0]].color;
                    d.event_play_sound();
                    break;
                }
            }
        } while (0);

        //
        // DELAY 
        //

        if (mode == CREATE_ROADS) {
            // CREATE_ROAGS mode: delay to complete CREATE_ROADS_CYCLE_TIME_US
            long end_time_us = microsec_timer();
            long delay_us = CREATE_ROADS_CYCLE_TIME_US - (end_time_us - start_time_us);
            microsec_sleep(delay_us);
        } else { 
            // all other modes delay for 10 ms
            microsec_sleep(10000); 
        }
    }

    return 0;
}

// -----------------  CREATE ROAD SLICE  -----------------------------------------------------------

void create_road_slice(class world &w, double x, double y, double dir)
{
    double dpy = -0.5 * cos((dir+90) * (M_PI/180.0));
    double dpx = 0.5 * sin((dir+90) * (M_PI/180.0));

    for (int i = -26; i <= 26; i++) {
        if (w.get_static_pixel(x+i*dpx, y+i*dpy) == display::GREEN) {
            w.set_static_pixel(x+i*dpx, y+i*dpy, display::BLACK);
        }
    }
    w.set_static_pixel(x,y,display::YELLOW);
}

// -----------------  EDIT PIXELS  -----------------------------------------------------------------

void edit_pixels(class world &w, bool start, int x_current, int y_current)
{
    int x_rect_low, x_rect_high, y_rect_low, y_rect_high;

    static int x_rect_low_save, x_rect_high_save, y_rect_low_save, y_rect_high_save;
    static int x_start, y_start;

    // if this event is start of motion then
    //   save the start pixel coords;
    //   init saved rect
    //   set this pixel
    // endif
    if (start) {
        x_start = x_current;
        y_start = y_current;

        x_rect_low_save = x_start;
        x_rect_high_save  = x_start;
        y_rect_low_save = y_start;
        y_rect_high_save  = y_start;

        w.set_static_pixel(x_start, y_start, edit_pixels_color_selection);
    }

    // determine rectangle coordinates using the start and current pixel coords
    x_rect_low = (x_start <  x_current ? x_start : x_current);
    x_rect_high  = (x_start >= x_current ? x_start : x_current);
    y_rect_low = (y_start <  y_current ? y_start : y_current);
    y_rect_high  = (y_start >= y_current ? y_start : y_current);

    // set additional pixels for where the rectangle has grown since last time
    if (x_rect_high > x_rect_high_save) {
        for (int x = x_rect_high_save+1; x <= x_rect_high; x++) {
            for (int y = y_rect_low; y <= y_rect_high; y++) {
                w.set_static_pixel(x, y, edit_pixels_color_selection);
            }
        }
    }
    if (x_rect_low < x_rect_low_save) {
        for (int x = x_rect_low_save-1; x >= x_rect_low; x--) {
            for (int y = y_rect_low; y <= y_rect_high; y++) {
                w.set_static_pixel(x, y, edit_pixels_color_selection);
            }
        }
    }
    if (y_rect_high > y_rect_high_save) {
        for (int y = y_rect_high_save+1; y <= y_rect_high; y++) {
            for (int x = x_rect_low; x <= x_rect_high; x++) {
                w.set_static_pixel(x, y, edit_pixels_color_selection);
            }
        }
    }
    if (y_rect_low < y_rect_low_save) {
        for (int y = y_rect_low_save-1; y >= y_rect_low; y--) {
            for (int x = x_rect_low; x <= x_rect_high; x++) {
                w.set_static_pixel(x, y, edit_pixels_color_selection);
            }
        }
    }

    // XXX restore pixels if the rectangle is smaller

    // save rect 
    x_rect_low_save = x_rect_low;
    x_rect_high_save  = x_rect_high;
    y_rect_low_save = y_rect_low;
    y_rect_high_save  = y_rect_high;
}    
