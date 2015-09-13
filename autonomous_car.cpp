// xxx search yyy
#include <cassert>
#include <iomanip>
#include <cmath>  // yyy check other includes in other files
#include <sstream>
#include <mutex>

#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;

const int NO_VALUE = 9999999;

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

autonomous_car::autonomous_car(display &display, world &world, int id, double x, double y, double dir, double speed, double max_speed)
    : car(display,world,id,x,y,dir,speed,max_speed)
{
    state = STATE_DRIVING;
    time_in_this_state_us = 0;
    obstruction = OBSTRUCTION_NONE;
    distance_road_is_clear = NO_VALUE;
    for (auto& xl : x_line) {
        xl = MAX_VIEW_WIDTH/2;
    }
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

void autonomous_car::draw_dashboard(int pid) 
{
    class display &d = get_display();
    double base_row = 3.3;
    std::ostringstream s;

    // call base class draw_dashboard 
    car::draw_dashboard(pid);

    // draw around the autonomous dashboard
    d.draw_set_color(display::WHITE);
    d.draw_rect(0,98,590,102,pid,2);

    // if update_controls has not yet been called then
    // do not put up the autonomous section of the dashboard
    if (distance_road_is_clear == NO_VALUE) {
        return;
    }

    // autonomous dash line 1: state
    if (get_failed()) {
        s.str("");
        s << "FAILED: " << get_failed_str();
        d.text_draw(s.str(), base_row+0, 1, pid, false, 0, 1);
    } else {
        d.text_draw(state_string(), base_row+0, 1, pid, false, 0, 1);
    }
    
    // autonomous dash line 2: distance road is clear
    s.str("");
    s << "ROAD CLR: " << distance_road_is_clear << " " << obstruction_string();
    d.text_draw(s.str(), base_row+1, 1, pid, false, 0, 1);
}

// -----------------  UPDATE CONTROLS VIRTUAL FUNCTION  -----------------------------

#define DEBUG_ID(x) \
    do { \
        DEBUG("ID " << get_id() << ": " << x); \
    } while (0)

void autonomous_car::update_controls(double microsecs)
{
    view_t view;

    // if failed do nothing
    if (get_failed()) {
        return;
    }

    // debug print seperator
    DEBUG_ID("--------------------------------------------------------\n");

    // get the front view
    world &w = get_world();
    w.get_view(get_x(), get_y(), get_dir(), MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, reinterpret_cast<unsigned char *>(view));

    // the front of car sometimes extends a little beyone yo-8;
    // check for this and clear these pixels
    for (int y_idx = yo-8; y_idx >= yo-9; y_idx--) {
        for (int x_idx = xo-5; x_idx <= xo+5; x_idx++) {
            if (view[y_idx][x_idx] == display::WHITE) {
                view[y_idx][x_idx] = display::BLACK;    
            }
            if (view[y_idx][x_idx] == display::BLUE) {
                view[y_idx][x_idx] = display::BLACK;    
            }
        }
    }

    // call scan_road to determine the center line for which the road is clear,
    // if distance road is clear is 0 then fail car
    scan_road(view);
    if (distance_road_is_clear == 0) {
        set_failed("COLLISION");
    }

    // set car steering and speed controls
    set_car_controls();

    // state machine
    time_in_this_state_us += microsecs;
    switch (state) {
    case STATE_DRIVING:
        if (get_speed() == 0) {
            if (distance_road_is_clear <= 5 && obstruction == OBSTRUCTION_STOP_LINE) {
                state_change(STATE_STOPPED_AT_STOP_LINE);
            } else if (distance_road_is_clear <= 5 && obstruction == OBSTRUCTION_END_OF_ROAD) {
                state_change(STATE_STOPPED_AT_END_OF_ROAD);
            } else if (distance_road_is_clear <= 5 && obstruction == OBSTRUCTION_REAR_VEHICLE) {
                state_change(STATE_STOPPED_AT_VEHICLE);
            } else if (distance_road_is_clear <= 5 && obstruction == OBSTRUCTION_FRONT_VEHICLE) {
                state_change(STATE_STOPPED_AT_VEHICLE);
            } else {
                state_change(STATE_STOPPED);
            }  
        }
        break;
    case STATE_STOPPED_AT_STOP_LINE:
        if (time_in_this_state_us > 1000000) {
            state_change(STATE_CONTINUING_FROM_STOP);
        }
        break;
    case STATE_STOPPED_AT_VEHICLE:
        if (get_speed() > 0) {
            state_change(STATE_DRIVING);
        }
        break;
    case STATE_STOPPED_AT_END_OF_ROAD:
        assert(get_speed() == 0);
        break;
    case STATE_STOPPED:
        if (get_speed() > 0) {
            state_change(STATE_DRIVING);
        }
        break;
    case STATE_CONTINUING_FROM_STOP:
        if (time_in_this_state_us > 1000000) {
            state_change(STATE_DRIVING);
        }
        break;
    default:
        assert(0);
        break;
    }
}

// TEST CASES WITH ONEE CAR
// - end of road at 45 degrees
// - end of road straight
// - stop sign
// - curvy road
// - straight road with gap
// - curvy road with gap
// - xxx tbd intersections

// TEST CASES WITH 5 CARS
// - repeat above

void autonomous_car::scan_road(view_t &view)
{
    int              max_x_line;
    int              y;
    enum obstruction obs;
    minigap_t        minigap;
    
    // init
    max_x_line     = 0;
    y              = yo - 8;  // in front of car
    obs            = OBSTRUCTION_NONE;
    minigap.y_last = NO_VALUE;
    minigap.slope  = 0;

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

        // if y is in mini gap then
        //   determine x line location based on mini gap slope
        // else if y is in a gap then
        //   determine x line location based on yyy
        // else
        //   determine x line location based on scanning for the center line
        // endif
        if (minigap.y_last != NO_VALUE) {
            x = x_last + minigap.slope;
            if (y == minigap.y_last) {
                minigap.y_last = NO_VALUE;
            }
        } else if (false) {
            x = NO_VALUE; // yyy
        } else {
            x = scan_across_for_center_line(view, y, x_last+slope);
        }

        // check if above code has determined location of center line, at y
        if (x != NO_VALUE) {
            // check for loop termination based on x,y,slope at limit
            if (y < 0 || x < 0 || x > MAX_VIEW_WIDTH-20 || abs(slope) > 1) {
                obs = OBSTRUCTION_NONE;
                break;
            }

            // check for loop termination based on road not clear 
            obs = scan_across_for_obstruction(view, y, x); 
            if (obs != OBSTRUCTION_NONE) {
                break;
            }

            // add location of center_line to x_line array
            x_line[max_x_line++] = x;
            y--;

            // continue
            continue;
        }

        // check for end of road at the next several values of y
        // if end of road detected
        //   set obsttuction to end of road
        //   break
        // endif
        if (scan_ahead_for_end_of_road(view, y, x_last+slope, slope)) {
            obs = OBSTRUCTION_END_OF_ROAD;
            break;
        }

        // check for mini gap by trying to locate center line at next several values of y;
        // if minigap found then
        //   continue
        // endif
        minigap = scan_ahead_for_minigap(view, y, x_last+slope, slope);
        if (minigap.y_last != NO_VALUE) {
            continue;
        }

        // yyy check for full gap

        // look ahead from the current y has not found any of:
        // - mini gap
        // - full gap
        // - end of road obstruction
        // so break out of the loop with no obstruction found
        obs = OBSTRUCTION_NONE;
        break;
    }

    // save private values
    distance_road_is_clear = max_x_line;
    obstruction = obs;

#ifdef ENABLE_LOGGING_AT_DEBUG_LEVEL
    // debug print 
    {
        std::ostringstream s;
        s << "return " << obstruction_string() << " " << distance_road_is_clear << " : ";
        for (int i = 0; i < distance_road_is_clear; i++) {
            s << x_line[i] << " ";
        }
        s << endl;
        DEBUG_ID(s.str());
    }
#endif
}

double autonomous_car::scan_across_for_center_line(view_t &view, int y, double x_double)
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

enum autonomous_car::obstruction autonomous_car::scan_across_for_obstruction(view_t &view, int y, double x_double)
{
    enum obstruction obs = OBSTRUCTION_NONE;
    int x = x_double;

    // yyy should scan across for N*sqrt(1+slope^2)

    if (y < 0 || x < 0 || x+10 >= MAX_VIEW_WIDTH) {
        return OBSTRUCTION_NONE;
    }

    for (int x_idx = x; x_idx <= x+10; x_idx++) {
        unsigned char pixel = view[y][x_idx];
        if (pixel != display::YELLOW && pixel != display::BLACK) {
            if (pixel == display::RED && state == STATE_CONTINUING_FROM_STOP) {
                continue;
            }

            if (pixel == display::GREEN) {
                obs = OBSTRUCTION_END_OF_ROAD;
            } else if (pixel == display::RED) {
                obs = OBSTRUCTION_STOP_LINE;
            } else if (pixel == display::WHITE) {
                obs = OBSTRUCTION_FRONT_VEHICLE;
            } else if (pixel == display::ORANGE) {
                obs = OBSTRUCTION_REAR_VEHICLE;
            } else if (pixel == display::BLUE || pixel == display::PINK) {
                obs = OBSTRUCTION_REAR_VEHICLE;
                continue; 
            } else {
                assert(0);
            }
            break;
        }
    }

    return obs;
}

bool autonomous_car::scan_ahead_for_end_of_road(view_t &view, int y, double x_double, double slope)
{
    for (int y_idx = y; y_idx >= y-3; y_idx--) {
        int x = x_double;
        if (y_idx < 0 || x-5 < 0 || x+5 >= MAX_VIEW_WIDTH) {
            return false;
        }
        for (int x_idx = x-5; x_idx <= x+5; x_idx++) {
            if (view[y_idx][x_idx] == display::GREEN) {
                return true;
            }
        }
        x_double += slope;
    }

    return false;
}

autonomous_car::minigap_t autonomous_car::scan_ahead_for_minigap(view_t &view, int y, double x, double slope)
{
    minigap_t minigap = {NO_VALUE, 0.0};

    for (int y_idx = y-1; y_idx >= y-3; y_idx--) {
        double x_scl;
        x_scl = scan_across_for_center_line(view, y_idx, x + (y - y_idx) * slope);
        if (x_scl != NO_VALUE) {
            minigap.y_last = y_idx+1;
            minigap.slope = (x_scl - x + slope) / (y - y_idx + 1);
            break;
        }
    }

    return minigap;
}

void autonomous_car::set_car_controls()
{
    assert(distance_road_is_clear != NO_VALUE);

    // steering control
    double steer_direction;

    if (get_speed() == 0) {
        steer_direction = 0;
    } else if (distance_road_is_clear <= 5) {
        steer_direction = get_steer_ctl();
    } else {
        // yyy the 20 should be a function of speed
        int steer_target = distance_road_is_clear > 20 ? 20 : distance_road_is_clear - 1;
        steer_direction = atan((x_line[steer_target] + 7 - xo) / (steer_target + 1)) * (180./M_PI);
    }
    set_steer_ctl(steer_direction);
    DEBUG_ID("set_car_control steer_dir " << steer_direction << endl);

    // speed control
    double speed_target;
    double current_speed;
    double speed_ctl_val;
    double adjusted_distance_road_is_clear;
    const double K_ACCEL = 1.5;
    const double K_DECEL = 5.0;

    adjusted_distance_road_is_clear = (double)(distance_road_is_clear-5) / 5280;
    if (adjusted_distance_road_is_clear < 0) {
        adjusted_distance_road_is_clear = 0;
    }

    // yyy comment this
    speed_target = sqrt(2. * (-MIN_SPEED_CTL*3600/4) * adjusted_distance_road_is_clear);   // mph
    if (speed_target > get_max_speed()) {
        speed_target = get_max_speed();
    }

    current_speed = get_speed();
    DEBUG_ID("set_car_control speed_target " << speed_target << " current_speed " << current_speed << endl);

    if (speed_target >= current_speed) {
        speed_ctl_val = (speed_target - current_speed) * K_ACCEL;
    } else {
        // yyy what about emergency braking
        speed_ctl_val = (speed_target - current_speed) * K_DECEL;
    }

    set_speed_ctl(speed_ctl_val);
    DEBUG_ID("set_car_control speed_ctl_val " << speed_ctl_val << endl);
}

// -----------------  MISC SUPPORT --------------------------------------------------

void autonomous_car::state_change(enum state new_state)
{
    state = new_state;
    time_in_this_state_us = 0;
}

const string autonomous_car::state_string()
{
    switch (state) {
    case STATE_DRIVING:
        return "DRIVING";
    case STATE_STOPPED_AT_STOP_LINE:
        return "STOPPED_AT_STOP_LINE";
    case STATE_STOPPED_AT_VEHICLE:
        return "STOPPED_AT_VEHICLE";
    case STATE_STOPPED_AT_END_OF_ROAD:
        return "STOPPED_AT_END_OF_ROAD";
    case STATE_STOPPED:
        return "STOPPED";
    case STATE_CONTINUING_FROM_STOP:
        return "CONTINUING_FROM_STOP";
    default:
        return "INVALID";
    }
}

const string autonomous_car::obstruction_string()
{
    switch (obstruction) {
    case OBSTRUCTION_NONE:
        return "NONE";
    case OBSTRUCTION_STOP_LINE:
        return "STOP_LINE";
    case OBSTRUCTION_REAR_VEHICLE:
        return "REAR_VEHICLE";
    case OBSTRUCTION_FRONT_VEHICLE:
        return "FRONT_VEHICLE";
    case OBSTRUCTION_END_OF_ROAD:
        return "END_OF_ROAD";
    default:
        return "INVALID";
    }
}
