#include <cassert>
#include <iomanip>
#include <cmath>  // xxx check other includes in other files
#include <sstream>

#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

autonomous_car::autonomous_car(display &display, world &world, int id, double x, double y, double dir, double speed, double max_speed)
    : car(display,world,id,x,y,dir,speed,max_speed)
{
    distance_road_is_clear = 0;  // xxx use NO_VALUE
    state = READY;
}

autonomous_car::~autonomous_car()
{
}

// -----------------  DRAW VIEW VIRTUAL FUNCTION  ---------------------------------

void autonomous_car::draw_view(int pid)
{
    static struct display::texture * t;
    view_t view;
    class display &d = get_display();
    class world &w = get_world();

    if (t == NULL) {
        t = d.texture_create(MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
        assert(t);
    }

    w.get_view(get_x(), get_y(), get_dir(), MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, reinterpret_cast<unsigned char *>(view));
    d.texture_set_rect(t, 0, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, reinterpret_cast<unsigned char *>(view), MAX_VIEW_WIDTH);
    d.texture_draw2(t, pid, 300-MAX_VIEW_WIDTH/2, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);

    d.text_draw("FRONT", 0, 0, pid, false, 0, 1, true);
}

// -----------------  DRAW DASHBOARD VIRTUAL FUNCTION  -----------------------------

void autonomous_car::draw_dashboard(int pid)  // xxx make seperate pid
{
    class display &d = get_display();

    // call base class draw_dashboard 
    // xxx show failed in car.cpp dash
    car::draw_dashboard(pid);

    // draw around the autonomous dashboard
    d.draw_set_color(display::WHITE);
    d.draw_rect(0,98,590,102,pid,2);

    // xxx autonomous dash content is tbd
    // - add things like the distance road is clear, stopped at sign
    // xxx d.text_draw("AUTONOMOUS DASH", 3.0, 0, pid, false, 0, 0, true);

    double base_row = 3.3;
    std::ostringstream s;

    d.text_draw(state_string(), base_row+0, 1, pid, false, 0, 1);
    
    s << "ROAD CLR " << distance_road_is_clear;
    d.text_draw(s.str(), base_row+1, 1, pid, false, 0, 1);
}


// -----------------  UPDATE CONTROLS VIRTUAL FUNCTION  -----------------------------

const int NO_VALUE = 9999999;

void autonomous_car::update_controls(double microsecs)
{
    view_t view;

    // if failed do nothing
    if (get_failed()) {
        return;
    }

    INFO("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

    // get the front view
    world &w = get_world();
    w.get_view(get_x(), get_y(), get_dir(), MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, reinterpret_cast<unsigned char *>(view));

    // xxx clear first
    for (int x_idx = xo-5; x_idx <= xo+5; x_idx++) {
        if (view[yo-8][x_idx] == display::WHITE) {
            INFO("CLEARING WHITE at y,x= " << yo-8 << " " << x_idx << endl);
            view[yo-8][x_idx] = display::BLACK;    
        }
        if (view[yo-8][x_idx] == display::BLUE) {
            INFO("CLEARING BLUE at y,x= " << yo-8 << " " << x_idx << endl);
            view[yo-8][x_idx] = display::BLACK;    
        }
    }

    for (int x_idx = xo-5; x_idx <= xo+5; x_idx++) {
        if (view[yo-9][x_idx] == display::WHITE) {
            INFO("2 ** CLEARING WHITE at y,x= " << yo-9 << " " << x_idx << endl);
            view[yo-9][x_idx] = display::BLACK;    
        }
        if (view[yo-9][x_idx] == display::BLUE) {
            INFO("2 ** CLEARING BLUE at y,x= " << yo-9 << " " << x_idx << endl);
            view[yo-9][x_idx] = display::BLACK;    
        }
    }
            
    // convert gap world coords to view coords,
    // if front of car is beyond the gap then clear gap
    // xxx gap is tbd

    // processing based on state of car
    if (stopped_at_stop_sign(view)) {
        // set state
        state = STOPPED_AT_STOP_LINE;

        INFO("stopped at stop sign\n");
        //xxx set_failed();
#if 0  // xxx later
        // car is stopped at stop sign ...

        // search for center lines
        search_for_center_lines(center_line);
        
        // if no center lines found then fail car
        if (!center_line[0].exists && !center_line[1].exists && !center_line[2].exists) {
            set_failed();
            return;
        }

        // pick a direction at random
        while (true) {
            static std::default_random_engine generator(microsec_timer();
            static std::uniform_int_distribution<int> choice(0,2);
            if (center_lines[choice(generator)].exists) {
                break;
            }
        }

        // set gap target to the begining of the chosen center line
        gap_target = center_lines[choice].begining;
        
        // set go_from_stop_sign
        go_from_stop_sign = xxx;
#endif
    } else {
        // car is not stopped at stop sign  ...

        // call scan_road to determine the center line for which the road is clear
        // xxx use vector
        int max_x_line;
        double x_line[MAX_VIEW_HEIGHT];
        scan_road(view, max_x_line, x_line);
        INFO("scan_road return: max_x_line " << max_x_line << " : ");
        for (int i = 0; i < max_x_line; i++) {
            cout << x_line[i] << " ";
        }
        cout << endl;

        // save distance road is clear for display on dashboard
        distance_road_is_clear = max_x_line;

        // if distance road is clear is 0 then fail car
        // xxx integrate this with state
        if (max_x_line == 0) {
            INFO("setting failed max_x_line 0\n");
            set_failed();  // xxx reason string?, and display on dashboard for base class
        }

        // set car steering and speed controls
        set_car_controls(max_x_line, x_line);

        // set state
        // xxx need APPROACHING_STOP_LINE and FAILED
        state = get_speed() > 0 ? TRAVELLING : STOPPED;
    }
}

bool autonomous_car::stopped_at_stop_sign(view_t &view)
{
    // if speed not zero return false
    if (get_speed() > 0) {
        return false;
    }

    // return true if stop line found neear front of car
    // xxx currently usng 5 feet  (12-8+1), tighten this up
    for (int y = yo-8; y >= yo-12; y--) {
        for (int x = xo+6; x >= xo-6; x--) {
            if (view[y][x] == display::YELLOW) {
                break;
            }
            if (view[y][x] == display::RED) {
                return true;
            }
        }
    }

    // stop line not found
    return false;
}

void autonomous_car::scan_road(view_t &view, int &max_x_line, double (&x_line)[MAX_VIEW_HEIGHT]) 
{
    int    y;
    int    minigap_y_end;
    double minigap_slope;
    
    // init
    max_x_line = 0;
    y = yo - 8;  // in front of car
    minigap_y_end = NO_VALUE;
    minigap_slope = 0;

    // this loop determines the x location of the center line, 
    while (true) {
        double slope, x_last, x;

        // determine the slope of the center line over the past 10 feet,
        // if that much has not yet been scanned then set slope to 0
        if (max_x_line > 10) {
            slope = (x_line[max_x_line-1] - x_line[max_x_line-(1+10)]) / 10;
        } else {
            slope = 0;
        }

        // determine the last x location of center line, 
        // if none yet then use expected
        if (max_x_line > 0) {
            x_last = x_line[max_x_line-1];
        } else {
            x_last = xo - 7;
        }
        INFO("y " << y << " x_last " << x_last << " slope " << slope << endl);

        // if y is in mini gap then
        //   determine x line location based on mini gap slope
        // else if y is in a gap then
        //   determine x line location based on xxx
        // else
        //   determine x line location based on scanning for the center line
        // endif
        if (minigap_y_end != NO_VALUE) {
            x = x_last + minigap_slope;
            if (minigap_y_end == y) {
                minigap_y_end = NO_VALUE;
            }
            INFO("minigap x " << x << endl);
        } else if (false) {
            x = NO_VALUE; // xxx
        } else {
            x = scan_for_center_line(view, y, x_last+slope);
            INFO("scan_for_center_line x " << x << endl);
        }

        // check if above code has determined location of center line, at y
        if (x != NO_VALUE) {
            // check for loop termination based on x,y,slope at limit
            if (y < 0 ||
                x < 0 || x > MAX_VIEW_WIDTH-20 ||
                abs(slope) > 1) 
            {
                INFO("term y,x,slope " << y << " " << x << " " << slope << endl);
                break;
            }

            // check for loop termination based on road not clear 
            // xxx should scan across for 13*sqrt(1+slope^2)
            // XXX might hit a green, maybe just lower the 13
            bool road_not_clear = false;
            int x_idx;  // xxx put decl back in loop
            for (x_idx = x; x_idx < x+13; x_idx++) {
                if (view[y][x_idx] != display::YELLOW && view[y][x_idx] != display::BLACK) {
                    road_not_clear = true;
                    break;
                }
            }
            if (road_not_clear) {
                INFO("road not clear, x_idx " << x_idx << " color " << (int)view[y][x_idx] << endl);
                break;
            }

            // add location of center_line to x_line array
            INFO("saving x_line[" << max_x_line << "] = " << x << endl);
            x_line[max_x_line++] = x;
            y--;

            // continue
            continue;
        }

        // check for mini gap by trying to locate center line at next several values of y;
        // note that center line can either be found in the view or found in start of a full gap
        // if found then
        //   set mini gap variables
        //   continue
        // endif
        INFO("checking for minigap at y " << y << endl);
        for (int y_idx = y-1; y_idx >= y-3; y_idx--) {
            double x_scl = scan_for_center_line(view, y_idx, x_last + (y - y_idx) * slope);
            if (x_scl != NO_VALUE) {
                minigap_y_end = y_idx+1;
                minigap_slope = (x_scl - x_last) / (y - y_idx);
            }
        }
        if (minigap_y_end != NO_VALUE) {
            INFO("found minigap_y_end " << minigap_y_end << " minigap_slope " << minigap_slope << endl);
            continue;
        }

        // xxx check for full gap

        // at this y we have not found a center line, minigap of full gap;
        // so we're done
        INFO("done because no value or gap\n");
        break;
    }
}

double autonomous_car::scan_for_center_line(view_t &view, int y, double x_double)
{
    int x = round(x_double);
    int x_found_start = NO_VALUE;
    int x_found_end   = NO_VALUE;

    if (y < 0 || x-5 < 0 || x+5 >= MAX_VIEW_WIDTH) {
        return NO_VALUE;
    }

    for (int x_idx = x-5; x_idx <= x+5; x_idx++) {
        if (view[y][x_idx] == display::YELLOW) {
            if (x_found_start == NO_VALUE) {
                x_found_start = x_idx;
            }
            x_found_end = x_idx;
        }
    }

    return (x_found_start == NO_VALUE ? NO_VALUE : (double)(x_found_start + x_found_end) / 2);
}

void autonomous_car::set_car_controls(int max_x_line, double (&x_line)[MAX_VIEW_HEIGHT])
{
    // steering control
    const int STEER_TARGET = 20;  // distance up the road
    double steer_direction;

    steer_direction = atan((x_line[STEER_TARGET] + 7 - xo) / STEER_TARGET) * (180./M_PI);
    set_steer_ctl(steer_direction);
    INFO("set_car_control steer_dir " << steer_direction << endl);

    // speed control
    double speed_target;
    double current_speed;
    double speed_ctl_val;
    double adjusted_distance_road_is_clear;
    const double K_ACCEL = 1.5;
    const double K_DECEL = 5.0;

    adjusted_distance_road_is_clear = (double)(max_x_line-5) / 5280;
    if (adjusted_distance_road_is_clear < 0) {
        adjusted_distance_road_is_clear = 0;
    }

    // xxx comment this
    speed_target = sqrt(2. * (-MIN_SPEED_CTL*3600/4) * adjusted_distance_road_is_clear);   // mph
    if (speed_target > get_max_speed()) {
        speed_target = get_max_speed();
    }

    current_speed = get_speed();
    INFO("set_car_control speed_target " << speed_target << " current_speed " << current_speed << endl);

    if (speed_target >= current_speed) {
        speed_ctl_val = (speed_target - current_speed) * K_ACCEL;
    } else {
        speed_ctl_val = (speed_target - current_speed) * K_DECEL;
    }

    set_speed_ctl(speed_ctl_val);
    INFO("set_car_control speed_ctl_val " << speed_ctl_val << endl);
}

// -----------------  MISC SUPPORT --------------------------------------------------

const string autonomous_car::state_string()
{
    switch (state) {
    case READY:
        return "READY";
    case TRAVELLING:
        return "TRAVELLING";
    case STOPPED:
        return "STOPPED";
    case APPROACHING_STOP_LINE:
        return "APPROACHING STOP LINE";
    case STOPPED_AT_STOP_LINE:
        return "STOPPED AT STOP LINE";
    case FAILED:
        return "FAILED";
    default:
        return "INVALID STATE";
    }
}

