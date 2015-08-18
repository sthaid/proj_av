#include <sstream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <unistd.h>
#include <string.h>

#include "display.h"
#include "world.h"
#include "fixed_control_car.h"
#include "logging.h"
#include "utils.h"

using std::thread;
using std::atomic;
using std::mutex;
using std::condition_variable;

// display and pane size  XXX panes for car display   1,2
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
#define PANE_CAR_DASHBOARD_HEIGHT   100

#define PANE_PGM_CTL_ID             3                        
#define PANE_PGM_CTL_X              820
#define PANE_PGM_CTL_Y              500
#define PANE_PGM_CTL_WIDTH          600
#define PANE_PGM_CTL_HEIGHT         250

#define PANE_MSG_BOX_ID             4
#define PANE_MSG_BOX_X              820    // pane id 4
#define PANE_MSG_BOX_Y              750
#define PANE_MSG_BOX_WIDTH          600
#define PANE_MSG_BOX_HEIGHT         50 

// world display pane center coordinates and zoom
double       center_x = world::WORLD_WIDTH / 2;
double       center_y = world::WORLD_HEIGHT / 2;
const double ZOOM_FACTOR = 1.1892071;
const double MAX_ZOOM    = 64.0 - .01;
const double MIN_ZOOM    = (1.0 / ZOOM_FACTOR) + .01;
double       zoom = 1.0; //XXX  / ZOOM_FACTOR;

// pane message box 
const int MAX_MESSAGE_AGE = 200;
const int DELAY_MICROSECS = 10000;
string    message = "";
int       message_age = MAX_MESSAGE_AGE;

// cars
typedef class fixed_control_car CAR;
const int     MAX_CAR = 1000;
CAR         * car[MAX_CAR];
int           max_car = 0;

// update car controls threads
const int          MAX_CAR_UPDATE_CONTROLS_THREAD = 10;
bool               car_update_controls_terminate = false;
atomic<int>        car_update_controls_idx(0);
atomic<int>        car_update_controls_completed(0); 
condition_variable car_update_controls_cv1;
mutex              car_update_controls_cv1_mtx;
condition_variable car_update_controls_cv2;
mutex              car_update_controls_cv2_mtx;
thread             car_update_control_thread_id[MAX_CAR_UPDATE_CONTROLS_THREAD];
void car_update_controls_thread(int id);

// -----------------  MAIN  ------------------------------------------------------------------------

int main(int argc, char **argv)
{
    enum mode { RUN, PAUSE };

    string     world_filename = "world.dat";
    enum mode  mode = PAUSE;
    bool       done = false;

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

    // call static initialization routines for the world and car classes
    world::static_init();
    car::static_init(d);

    // create the world
    world w(d,world_filename);
    message = w.read_ok() ? "READ SUCCESS" : "READ FAILURE";
    message_age = 0;

    // create cars
    for (double dir = 0; dir < 360; dir += 1) {
        car[max_car++] = new CAR(d,w,2048,2048,dir, 10);
    }

    // create threads to update car controls
    for (int i = 0; i < MAX_CAR_UPDATE_CONTROLS_THREAD; i++) {
        car_update_control_thread_id[i] = thread(car_update_controls_thread, i);
    }

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
            car_update_controls_idx = 0;  // xxx check the order of these
            car_update_controls_completed = 0;
            car_update_controls_cv1.notify_all();
            std::unique_lock<std::mutex> car_update_controls_cv2_lck(car_update_controls_cv2_mtx);
            while (car_update_controls_completed != max_car) {
                car_update_controls_cv2.wait(car_update_controls_cv2_lck);
            }
            car_update_controls_cv2_lck.unlock(); // xxx not needed
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

        // draw car state
        car[0]->draw(PANE_CAR_VIEW_ID, PANE_CAR_DASHBOARD_ID);

        // draw the message box
        // XXX age should be seconds not cycles
        if (message_age < MAX_MESSAGE_AGE) {
            d.text_draw(message, 0, 0, PANE_MSG_BOX_ID);
            message_age++;
        }

        // draw and register events
        int eid_quit_win = d.event_register(display::ET_QUIT);
        int eid_pan      = d.event_register(display::ET_MOUSE_MOTION, 0);
        int eid_zoom     = d.event_register(display::ET_MOUSE_WHEEL, 0);
        int eid_run      = d.text_draw("RUN",   0, 0, PANE_PGM_CTL_ID, true, 'r');      
        int eid_pause    = d.text_draw("PAUSE", 0, 7, PANE_PGM_CTL_ID, true, 's');      

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

    // terminate the car_update_control_threads
    car_update_controls_terminate = true;
    car_update_controls_cv1.notify_all();
    for (auto& th : car_update_control_thread_id) {
        th.join();
    }

    return 0;
}

// -----------------  CAR UPDATE CONTROLS THREAD----------------------------------------------------

void car_update_controls_thread(int id) 
{
    int idx;

    while (true) {
        // wait for request
        std::unique_lock<std::mutex> car_update_controls_cv1_lck(car_update_controls_cv1_mtx);
        while (car_update_controls_idx >= max_car && !car_update_controls_terminate) {
            car_update_controls_cv1.wait(car_update_controls_cv1_lck);
        }
        car_update_controls_cv1_lck.unlock();

        // if terminate requested then break
        if (car_update_controls_terminate) {
            break;
        }

        // update car controls
        while ((idx = car_update_controls_idx.fetch_add(1)) < max_car) {
            car[idx]->update_controls(DELAY_MICROSECS);
            car_update_controls_completed++;
        }

        // if this thread finished up the work then notify main that we're done
        if (car_update_controls_completed == max_car) {
            car_update_controls_cv2.notify_one();
        }
    }
}
