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
#include <thread>
#include <atomic>
#include <condition_variable>
#include <random>

#include <unistd.h>  // for getopt

#include "display.h"
#include "world.h"
#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

using std::thread;
using std::atomic;
using std::mutex;
using std::condition_variable;
using std::ostringstream;
using std::istringstream;

// display and pane size 
#define DISPLAY_WIDTH               1420
#define DISPLAY_HEIGHT              800

#define PANE_WORLD_ID               0
#define PANE_WORLD_X                0 
#define PANE_WORLD_Y                0
#define PANE_WORLD_WIDTH            800
#define PANE_WORLD_HEIGHT           800

#define PANE_CAR_VIEW_ID            1
#define PANE_CAR_VIEW_X             820
#define PANE_CAR_VIEW_Y             0
#define PANE_CAR_VIEW_WIDTH         600
#define PANE_CAR_VIEW_HEIGHT        400

#define PANE_CAR_DASHBOARD_ID       2                    
#define PANE_CAR_DASHBOARD_X        820
#define PANE_CAR_DASHBOARD_Y        400
#define PANE_CAR_DASHBOARD_WIDTH    600
#define PANE_CAR_DASHBOARD_HEIGHT   200

#define PANE_PGM_CTL_ID             3                        
#define PANE_PGM_CTL_X              820
#define PANE_PGM_CTL_Y              620
#define PANE_PGM_CTL_WIDTH          600
#define PANE_PGM_CTL_HEIGHT         130

#define PANE_MSG_BOX_ID             4
#define PANE_MSG_BOX_X              820 
#define PANE_MSG_BOX_Y              750
#define PANE_MSG_BOX_WIDTH          600
#define PANE_MSG_BOX_HEIGHT         50 

// world display pane center coordinates and zoom
double       center_x = world::WORLD_WIDTH / 2;
double       center_y = world::WORLD_HEIGHT / 2;
const double ZOOM_FACTOR = 1.1892071;
const double MAX_ZOOM    = 256.0 - .01;
const double MIN_ZOOM    = (1.0 / ZOOM_FACTOR) + .01;
double       zoom = 1.0;

// simulation cycle time
const int CYCLE_TIME_US = 50000;  // 50 ms

// pane message box 
const int MAX_MESSAGE_TIME_US = 1000000;
string    message = "";
long      message_start_time_us;

// cars
const int     MAX_CAR = 300; 
class car   * car[MAX_CAR];
int           dashboard_and_view_idx = -1;
int           launch_pending = 0;
bool launch_new_car(display &d, world &w);
int get_next_dashboard_and_view_idx(int id);

// update car controls threads
const int          MAX_CAR_UPDATE_CONTROLS_THREAD = 10;
bool               car_update_controls_terminate = false;
atomic<int>        car_update_controls_idx(0);
atomic<int>        car_update_controls_completed(0); 
condition_variable car_update_controls_cv1;
mutex              car_update_controls_cv1_mtx;
condition_variable car_update_controls_cv2;
mutex              car_update_controls_cv2_mtx;
thread             car_update_controls_thread_id[MAX_CAR_UPDATE_CONTROLS_THREAD];
void car_update_controls_thread(int id);

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

    // get options, and args
    while (true) {
        char opt_char = getopt(argc, argv, "n:");
        if (opt_char == -1) {
            break;
        }
        switch (opt_char) {
        case 'n': {
            istringstream s(optarg);
            s >> launch_pending;
            if (s.fail() || !s.eof() || launch_pending < 0 || launch_pending > MAX_CAR) { 
                ERROR("invalid launch_pending '" << s.str() << "', max=" << MAX_CAR << endl);
                return 1;
            }
            break; }
        default:
            return 1;
        }
    }
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

    // create threads to update car controls
    car_update_controls_idx = MAX_CAR;
    for (int i = 0; i < MAX_CAR_UPDATE_CONTROLS_THREAD; i++) {
        car_update_controls_thread_id[i] = thread(car_update_controls_thread, i);
    }

    //
    // MAIN LOOP
    //

    enum mode { RUN, STOP, STEP };
    enum mode  mode = STOP;
    bool       done = false;

    while (!done) {
        //
        // STORE THE START TIME
        //

        long start_time_us = microsec_timer();

        //
        // CAR SIMULATION
        // 

        // launch cars
        if (launch_pending > 0 && launch_new_car(d,w)) {
            launch_pending--;
        }

        // update all car mechanics: position, direction, speed
        if (mode == RUN || mode == STEP) {            
            for (int i = 0; i < MAX_CAR; i++) {
                if (car[i] == NULL) {
                    continue;
                }
                car[i]->update_mechanics(CYCLE_TIME_US);
            }
        }

        // update car positions in the world 
        w.place_object_init();
        for (int i = 0; i < MAX_CAR; i++) {
            if (car[i] == NULL) {
                continue;
            }
            car[i]->place_car_in_world();
        }

        // update all car controls: steering and speed
        if (mode == RUN || mode == STEP) {            
            std::unique_lock<std::mutex> car_update_controls_cv2_lck(car_update_controls_cv2_mtx);
            car_update_controls_completed = 0;
            car_update_controls_idx = 0;
            car_update_controls_cv1.notify_all();
            while (car_update_controls_completed != MAX_CAR) {
                car_update_controls_cv2.wait(car_update_controls_cv2_lck);
            }
            car_update_controls_cv2_lck.unlock(); // not needed, unique_lock automatically unlocks
        }

        //
        // DISPLAY UPDATE 
        // 

        // start display update
        d.start(PANE_WORLD_X,         PANE_WORLD_Y,         PANE_WORLD_WIDTH,         PANE_WORLD_HEIGHT,
                PANE_CAR_VIEW_X,      PANE_CAR_VIEW_Y,      PANE_CAR_VIEW_WIDTH,      PANE_CAR_VIEW_HEIGHT,
                PANE_CAR_DASHBOARD_X, PANE_CAR_DASHBOARD_Y, PANE_CAR_DASHBOARD_WIDTH, PANE_CAR_DASHBOARD_HEIGHT,
                PANE_PGM_CTL_X,       PANE_PGM_CTL_Y,       PANE_PGM_CTL_WIDTH,       PANE_PGM_CTL_HEIGHT,
                PANE_MSG_BOX_X,       PANE_MSG_BOX_Y,       PANE_MSG_BOX_WIDTH,       PANE_MSG_BOX_HEIGHT);

        // draw world 
        w.draw(PANE_WORLD_ID,center_x,center_y,zoom);

        // draw car front view and dashboard
        if (dashboard_and_view_idx != -1) {
            car[dashboard_and_view_idx]->draw_view(PANE_CAR_VIEW_ID);
            car[dashboard_and_view_idx]->draw_dashboard(PANE_CAR_DASHBOARD_ID);
        }

        // draw pointers to all cars 
        for (int i = 0; i < MAX_CAR; i++) {
            if (car[i] == NULL) {
                continue;
            }

            double pixel_x, pixel_y;
            int ptr_size = 3 * zoom;
            if (ptr_size < 7) {
                ptr_size = 7;
            }
            enum display::color color;
            color = (i == dashboard_and_view_idx ? display::WHITE :
                     car[i]->get_failed()        ? display::PINK :
                                                   display::PURPLE);
            w.cvt_coord_world_to_pixel(car[i]->get_x(), car[i]->get_y(), pixel_x, pixel_y);
            d.draw_set_color(color);
            d.draw_pointer(pixel_x*PANE_WORLD_WIDTH, pixel_y*PANE_WORLD_HEIGHT, ptr_size, PANE_WORLD_ID);
        }

        // draw the message box
        if (microsec_timer() - message_start_time_us < MAX_MESSAGE_TIME_US) {
            d.text_draw(message, 0, 0, PANE_MSG_BOX_ID);
        }

        // draw and register events
        int eid_quit_win  = d.event_register(display::ET_QUIT);
        int eid_pan       = d.event_register(display::ET_MOUSE_LEFT_MOTION, 0);
        int eid_zoom      = d.event_register(display::ET_MOUSE_WHEEL, 0);
        int eid_run       = d.text_draw("RUN",    0, 0,  PANE_PGM_CTL_ID, true, 'r');
        int eid_stop      = d.text_draw("STOP",   0, 5,  PANE_PGM_CTL_ID, true, 's');
        int eid_launch    = d.text_draw("LAUNCH", 0, 11, PANE_PGM_CTL_ID, true, 'l');      
        int eid_delete    = d.text_draw("DEL",    0, 19, PANE_PGM_CTL_ID, true, 'd');      
        int eid_step      = d.text_draw("STEP",   1, 19, PANE_PGM_CTL_ID, true, 'd');      
        int eid_wp_click = d.event_register(display::ET_MOUSE_RIGHT_CLICK, PANE_WORLD_ID);
        int eid_vp_click = d.event_register(display::ET_MOUSE_RIGHT_CLICK, PANE_CAR_VIEW_ID);
        int eid_dp_click = d.event_register(display::ET_MOUSE_RIGHT_CLICK, PANE_CAR_DASHBOARD_ID);

        // display mode
        d.text_draw(mode == RUN ? "RUNNING" : mode == STEP ? "STEPPING" : "STOPPED", 
                    1, 0, PANE_PGM_CTL_ID);

        // display number cars: active, failed, and pending
        int failed_count = 0;
        int active_count = 0;
        for (int i = 0; i < MAX_CAR; i++) {
            if (car[i] == NULL) {
                continue;
            }
            if (!car[i]->get_failed()) {
                active_count++;
            } else {
                failed_count++;
            } 
        }
        ostringstream s;
        s << "ACTV " << active_count << " FAIL " << failed_count << " PEND " << launch_pending;
        d.text_draw(s.str(), 2, 0, PANE_PGM_CTL_ID);

        // finish, updates the display
        d.finish();

        // if mode is step then set to stop
        if (mode == STEP) {
            mode = STOP;
        }

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
            if (event.eid == eid_run) {
                mode = RUN;
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_stop) {
                mode = STOP;
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_step) {
                mode = STEP;
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_launch) {
                launch_pending++;
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_delete) {
                if (dashboard_and_view_idx != -1) {
                    int id = car[dashboard_and_view_idx]->get_id();
                    delete car[dashboard_and_view_idx];
                    car[dashboard_and_view_idx] = NULL;
                    dashboard_and_view_idx = get_next_dashboard_and_view_idx(id);
                }
                d.event_play_sound();
                break;
            }
            if (event.eid == eid_wp_click) {
                int x,y;
                w.cvt_coord_pixel_to_world((double)event.click.x/PANE_WORLD_WIDTH,
                                           (double)event.click.y/PANE_WORLD_HEIGHT,
                                           x, y);
                for (int i = 0; i < MAX_CAR; i++) {
                    if (car[i] == NULL) {
                        continue;
                    }
                    if (x >= car[i]->get_x() - 7 &&
                        x <= car[i]->get_x() + 7 &&
                        y >= car[i]->get_y() - 7 &&
                        y <= car[i]->get_y() + 7)
                    {
                        dashboard_and_view_idx = i;
                        d.event_play_sound();
                        break;
                    }
                }
                break;
            }
            if (event.eid == eid_vp_click || event.eid == eid_dp_click) {
                int id = (dashboard_and_view_idx != -1
                          ? car[dashboard_and_view_idx]->get_id()
                          : 0);
                dashboard_and_view_idx = get_next_dashboard_and_view_idx(id);
                d.event_play_sound();
                break;
            }
        } while(0);

        //
        // DELAY TO COMPLETE THE TARGET CYCLE TIME
        //

        // delay to complete CYCLE_TIME_US
        long end_time_us = microsec_timer();
        long delay_us = CYCLE_TIME_US - (end_time_us - start_time_us);
        microsec_sleep(delay_us);

#if 1
        // oncer every 10 secs, debug print this cycle's processing tie
        static int count;
        if (++count == 10000000 / CYCLE_TIME_US) {
            count = 0;
            INFO("PROCESSING TIME = " << end_time_us-start_time_us << " us" << endl);
        }
#endif
    }

    //
    // TERMINATE CAR_UPDATE_CONTROLS_THREADS   
    //

    car_update_controls_terminate = true;
    car_update_controls_cv1.notify_all();
    for (auto& th : car_update_controls_thread_id) {
        th.join();
    }

    return 0;
}

// -----------------  LAUNCH NEW CAR  --------------------------------------------------------------

bool launch_new_car(display &d, world &w)
{
    //XXX const int xo = 2055;
    //const int yo = 2048;
    //const int dir = 0;
    const int xo = 450;
    const int yo = 2620;
    const int dir = 335;
    const int speed = 0;

#if 0 // XXX
    // check for clear to launch
    for (int y = yo; y >= yo-12; y--) {
        if (w.get_world_pixel(xo,y) != display::BLACK) {
            return false;
        }
    }
#endif

    // find a free slot in car array
    int idx;
    for (idx = 0; idx < MAX_CAR; idx++) {
        if (car[idx] == NULL) {
            break;
        }
    }
    if (idx == MAX_CAR) {
        return false;
    }

    // choose the car's max speed at random, in range 30 to 50 mph
    static std::default_random_engine generator(microsec_timer()); 
    static std::uniform_int_distribution<int> random_uniform_30_to_50(30,50);
    int max_speed = random_uniform_30_to_50(generator);

    // choose the car's id
    static int id;
    id++;

    // create the car
    car[idx] = new class autonomous_car(d, w, id, xo, yo, dir, speed, max_speed);

    // if dashboard display is not active then display this car
    if (dashboard_and_view_idx == -1) {
        dashboard_and_view_idx = idx;
    }

    // return success
    return true;
}

// -----------------  GET NEXT DASHBOARD AND VIEW IDX  ---------------------------------------------

int get_next_dashboard_and_view_idx(int id_arg)
{
    int min_id_idx = -1;
    int min_id_greater_than_id_arg_idx = -1;
    int min_id = 99999999;
    int min_id_greater_than_id_arg = 99999999;

    // loop over all possible cars
    for (int idx = 0; idx < MAX_CAR; idx++) {
        // if car doesn't exist then continue
        if (car[idx] == NULL) {
            continue;
        }

        // get car's id
        int id = car[idx]->get_id();

        // save the minimum car id
        if (id < min_id) {
            min_id = id;
            min_id_idx = idx;
        }

        // save the minimum car id that is greater than id_arg
        if (id > id_arg && id < min_id_greater_than_id_arg) {
            min_id_greater_than_id_arg = id;
            min_id_greater_than_id_arg_idx = idx;
        }
    }

    // return the minimum car id that is greater than id arg, if that exists, 
    // else return the minimum car id; if there are no cars then -1 is returned
    return (min_id_greater_than_id_arg_idx != -1 
            ? min_id_greater_than_id_arg_idx
            : min_id_idx);
}
        
// -----------------  CAR UPDATE CONTROLS THREAD----------------------------------------------------

void car_update_controls_thread(int id) 
{
    int idx;

    while (true) {
        // wait for request
        std::unique_lock<std::mutex> car_update_controls_cv1_lck(car_update_controls_cv1_mtx);
        while (car_update_controls_idx >= MAX_CAR && !car_update_controls_terminate) {
            car_update_controls_cv1.wait(car_update_controls_cv1_lck);
        }
        car_update_controls_cv1_lck.unlock();

        // if terminate requested then break
        if (car_update_controls_terminate) {
            break;
        }

        // update car controls
        while ((idx = car_update_controls_idx.fetch_add(1)) < MAX_CAR) {
            if (car[idx] != NULL) {
                car[idx]->update_controls(CYCLE_TIME_US);
            }
            car_update_controls_completed++;
        }

        // if this thread finished up the work then notify main that we're done
        if (car_update_controls_completed == MAX_CAR) {
            car_update_controls_cv2_mtx.lock();
            car_update_controls_cv2.notify_one();
            car_update_controls_cv2_mtx.unlock();
        }
    }
}
