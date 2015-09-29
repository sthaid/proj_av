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

#include <cassert>
#include <iomanip>
#include <sstream>
#include <cmath>  
#include <random>

#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;

#define INFO_ID(x) \
    do { \
        INFO("ID " << get_id() << ": " << x); \
    } while (0)
#define WARNING_ID(x) \
    do { \
        WARNING("ID " << get_id() << ": " << x); \
    } while (0)
#define ERROR_ID(x) \
    do { \
        ERROR("ID " << get_id() << ": " << x); \
    } while (0)
#define DEBUG_ID(x) \
    do { \
        DEBUG("ID " << get_id() << ": " << x); \
    } while (0)

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
    fullgap.valid = false;
    continuing_from_stop_left_is_possible = false;
    continuing_from_stop_straight_is_possible = false;
    continuing_from_stop_right_is_possible = false;
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
#if 0
    d.texture_draw2(t, pid, 300-MAX_VIEW_WIDTH/2, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
#else
    d.texture_draw2(t, pid);
#endif

    d.text_draw("AUTONOMOUS CAR", 0, 0, pid, false, 0, 1, true);
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
    d.text_draw(state_string(state), base_row+0, 1, pid, false, 0, 1);

    // autonomous dash line 1: continuing from stop possible directions
    if (state == STATE_CONTINUING_FROM_STOP_LINE) {
        d.text_draw(continuing_from_stop_left_is_possible ? "L" : "-",
                    base_row+0, 28, pid, false, 0, 1);
        d.text_draw(continuing_from_stop_straight_is_possible ? "S" : "-",
                    base_row+0, 29, pid, false, 0, 1);
        d.text_draw(continuing_from_stop_right_is_possible ? "R" : "-",
                    base_row+0, 30, pid, false, 0, 1);
    }
    
    // autonomous dash line 2: distance road is clear
    s.str("");
    s << "ROAD CLR: " << distance_road_is_clear << " " << obstruction_string(obstruction);
    d.text_draw(s.str(), base_row+1, 1, pid, false, 0, 1);
}

// -----------------  UPDATE CONTROLS VIRTUAL FUNCTION  -----------------------------

void autonomous_car::update_controls(double microsecs)
{
    view_t view;
    
    // if failed do nothing
    if (get_failed()) {
        return;
    }

    // debug print seperator
    DEBUG_ID("--------------------------------------------------------" << endl);

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
        // XXX print car data 
    }
    if (get_failed()) {
        return;
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
            state_change(STATE_CONTINUING_FROM_STOP_LINE);
            // Should not usually have fullgap.valid when stopped at stop line, because scan road
            // should have seen the stop line and stopped scanning. However, if the stop line is not
            // pronounced, then scan road can sometimes not see it and init a fullgap following the
            // stop line. This check clears the fullgap if that has occurred.
            if (fullgap.valid) {
                WARNING_ID("clearing fullgap.valid when continuing from stop" << endl);
                fullgap.valid = false;
            }
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
    case STATE_CONTINUING_FROM_STOP_LINE:
        if (time_in_this_state_us > 4000000) {
            state_change(STATE_DRIVING);
        }
        break;
    default:
        assert(0);
        break;
    }
}

void autonomous_car::scan_road(view_t &view)
{
    int              max_x_line;
    int              y;
    enum obstruction obs;
    int              minigap_y_last;
    double           minigap_slope;
    string           minigap_type_str;
    bool             end_of_road_detected;
    
    // init locals
    max_x_line           = 0;
    y                    = yo - 8;  // in front of car
    obs                  = OBSTRUCTION_NONE;
    minigap_y_last       = NO_VALUE;
    minigap_slope        = 0;
    end_of_road_detected = false;

    // initial debug prints
    DEBUG_ID("world - y,x " << get_y() << " " << get_x() << " dir " << get_dir() << endl);
    DEBUG_ID("state - " << state_string(state) << endl);

    // if a fullgap is valid then
    //  . convert its saved world coordinates to the coords in the current view
    //  . if the end of the fullgap is before the begining of this view then 
    //    the fullgap is no longer valid
    // endif
    if (fullgap.valid) {
        coord_convert_fixed_to_view(fullgap.y_start_fixed, fullgap.x_start_fixed, 
                                    fullgap.y_start_view, fullgap.x_start_view);
        coord_convert_fixed_to_view(fullgap.y_end_fixed, fullgap.x_end_fixed, 
                                    fullgap.y_end_view, fullgap.x_end_view);
        DEBUG_ID("fullgap - updated, " 
                 << "VIEW "  << fullgap.y_start_view << " " << fullgap.x_start_view << " -> "
                             << fullgap.y_end_view << " " << fullgap.x_end_view << " "
                 << "FIXED " << fullgap.y_start_fixed << " " << fullgap.x_start_fixed << " -> "
                             << fullgap.y_end_fixed << " " << fullgap.x_end_fixed 
                 << endl);
        if (fullgap.y_end_view >= y) {
            DEBUG_ID("fullgap - complete" << endl);
            fullgap.valid = false;
        }
    }

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

        // if slope too large then scan is complete
        if (fabs(slope) > 1) {
            DEBUG_ID("done - at y = " << y << ", because of slope " << slope << endl);
            obs = OBSTRUCTION_NONE;
            break;
        }

        // determine the last x location of center line, 
        // if none yet then use expected
        if (max_x_line > 0) {
            x_last = x_line[max_x_line-1];
        } else {
            x_last = xo - 7;
        }

        // if y is in minigap then
        //   determine x line location based on minigap slope
        // else if end of road detected then
        //   determine x line location based on slope
        // else if y is in a fullgap then
        //   determine x line location based on fullgap slope
        // else
        //   determine x line location based on scanning for the center line
        // endif
        if (minigap_y_last != NO_VALUE) {
            x = x_last + minigap_slope;
            DEBUG_ID("minigap - got y,x = " << y << " " << x << " - minigap_slope " << minigap_slope << endl);
            if (y == minigap_y_last) {
                DEBUG_ID("minigap - complete" << endl);
                minigap_y_last = NO_VALUE;
            }
        } else if (end_of_road_detected) {
            x = x_last + slope;
            DEBUG_ID("end_of_road_detected - got y,x = " << y << " " << x << " - slope " << slope << endl);
        } else if (fullgap.valid && y < fullgap.y_start_view && y > fullgap.y_end_view) {
            double fullgap_slope = -(fullgap.x_end_view - fullgap.x_start_view) / 
                                    (fullgap.y_end_view - fullgap.y_start_view);
            x = x_last + fullgap_slope;
            DEBUG_ID("fullgap - got y,x = " << y << " " << x << " - fullgap_slope " << fullgap_slope << endl);
        } else {
            x = scan_across_for_center_line(view, y, x_last+slope);
            if (x != NO_VALUE) {
                DEBUG_ID("scan - got y,x = " << y << " " << x << " - slope " << slope << endl);
            } else {
                DEBUG_ID("scan - got y,x = " << y << " NO_VALUE" << " - slope " << slope << endl);
            }
        }

        // check if above code has determined location of center line, at y
        if (x != NO_VALUE) {
            // if x too close to edge then scan is complete
            if (x < 20 || x > MAX_VIEW_WIDTH-1 - 20) {
                DEBUG_ID("done - at y = " << y << ", because x " << x << " too close to edge" << endl);
                obs = OBSTRUCTION_NONE;
                break;
            }

            // check for loop termination based on road not clear 
            obs = scan_across_for_obstruction(view, y, x); 
            if (obs != OBSTRUCTION_NONE) {
                DEBUG_ID("done - at y = " << y << ", because " << obstruction_string(obs) << endl);
                break;
            }

            // add location of center_line to x_line array
            x_line[max_x_line++] = x;
            y--;

            // if y is too close to view top then scan is complete
            if (y < 20) {
                DEBUG_ID("done - at y = " << y << ", because y " << y << " too close to top" << endl);
                obs = OBSTRUCTION_NONE;
                break;
            }

            // continue
            continue;
        }

        // check for mini gap by trying to locate center line at next several values of y;
        // if minigap found then
        //   continue
        // endif
        scan_ahead_for_minigap(view, y, x_last+slope, slope, minigap_y_last, minigap_slope, minigap_type_str);
        if (minigap_y_last != NO_VALUE) {
            DEBUG_ID("minigap - start," << 
                     " minigap_y_last = " << minigap_y_last << 
                     " minigap_slope = " << minigap_slope << 
                     " minigap_type_str = " << minigap_type_str << endl);
            continue;
        }

        // check for end of road at the next several values of y
        // if end of road detected
        //   continue
        // endif
        end_of_road_detected = scan_ahead_for_end_of_road(view, y, x_last+slope, slope);
        if (end_of_road_detected) {
            DEBUG_ID("end_of_road_detected - start" << endl);
            continue;
        }

        // if there isn't currently a valid fullgap then
        //    scan ahead for center line(s) that continue straight, left, or right;
        //    if continuing from a stop and the fullgap is close to the car then
        //       choose either one of the continuation center lines at random
        //    else
        //       choose the straight continuation center line
        //    endif
        //    if a continuation center line has been identified then
        //       initialize the fullgap struct
        //       continue scanning the road
        //    endif
        // endif
        if (!fullgap.valid) {
            int y_straight, x_straight, y_left, x_left, y_right, x_right;
            int fullgap_y_end_view = NO_VALUE, fullgap_x_end_view = NO_VALUE;
            string choice("none");

            scan_ahead_for_continuing_center_lines(view, y, x_last+slope, slope, 
                                                   y_straight, x_straight, 
                                                   y_left, x_left, 
                                                   y_right, x_right);

            if (state == STATE_CONTINUING_FROM_STOP_LINE && y > yo - 25) {
                continuing_from_stop_left_is_possible = (y_left != NO_VALUE);
                continuing_from_stop_straight_is_possible = (y_straight != NO_VALUE);
                continuing_from_stop_right_is_possible = (y_right != NO_VALUE);
                if (y_straight != NO_VALUE || y_left != NO_VALUE || y_right != NO_VALUE) {
                    while (true) {
                        static std::default_random_engine generator(microsec_timer());
                        static std::uniform_int_distribution<int> rand_0_to_2(0,2);
                        int n = rand_0_to_2(generator);
                        assert(n >= 0 && n <= 2);
                        if (n == 0 && y_straight != NO_VALUE) {
                            fullgap_y_end_view = y_straight;
                            fullgap_x_end_view = x_straight;
                            choice = "straight";
                            break;
                        }
                        if (n == 1 && y_left != NO_VALUE) {
                            fullgap_y_end_view = y_left;
                            fullgap_x_end_view = x_left;
                            choice = "left";
                            break;
                        }
                        if (n == 2 && y_right != NO_VALUE) {
                            fullgap_y_end_view = y_right;
                            fullgap_x_end_view = x_right;
                            choice = "right";
                            break;
                        }
                    }
                }
            } else {
                if (y_straight != NO_VALUE) {
                    fullgap_y_end_view = y_straight;
                    fullgap_x_end_view = x_straight;
                    choice = "straight";
                }
            }

            if (fullgap_y_end_view != NO_VALUE) {
                fullgap.y_start_view = y + 1;
                fullgap.x_start_view = x_last;
                fullgap.y_end_view   = fullgap_y_end_view;
                fullgap.x_end_view   = fullgap_x_end_view;
                coord_convert_view_to_fixed(fullgap.y_start_view, fullgap.x_start_view, 
                                            fullgap.y_start_fixed, fullgap.x_start_fixed);
                coord_convert_view_to_fixed(fullgap.y_end_view, fullgap.x_end_view, 
                                            fullgap.y_end_fixed, fullgap.x_end_fixed);
                fullgap.valid   = true;
                DEBUG_ID("fullgap - identified - " 
                         << (y_left != NO_VALUE ? 'L' : 'x')
                         << (y_straight != NO_VALUE ? 'S' : 'x')
                         << (y_right != NO_VALUE ? 'R' : 'x')
                         << " - CHOICE is " << choice << " - "
                         << "VIEW "  << fullgap.y_start_view << " " << fullgap.x_start_view << " -> "
                                     << fullgap.y_end_view << " " << fullgap.x_end_view << " "
                         << "FIXED " << fullgap.y_start_fixed << " " << fullgap.x_start_fixed << " -> "
                                     << fullgap.y_end_fixed << " " << fullgap.x_end_fixed 
                         << "\a" // alarm beep
                         << endl);
                continue;
            }
        }

        // scan is complete because no center line found at current y and no mingap or fullgap
        DEBUG_ID("done - no minigap or fullgap" << endl);
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
        s << "return " << obstruction_string(obstruction) << " " << distance_road_is_clear << " : ";
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

    assert(y >= 0 && y < MAX_VIEW_HEIGHT);
    assert(x-5 >= 0 && x+5 < MAX_VIEW_WIDTH);

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
    int x = round(x_double);

    // XXX should scan across for N*sqrt(1+slope^2)

    assert(y >= 0 && y < MAX_VIEW_HEIGHT);
    assert(x >= 0 && x+10 < MAX_VIEW_WIDTH);

    for (int x_idx = x; x_idx <= x+10; x_idx++) {
        unsigned char pixel = view[y][x_idx];
        if (pixel != display::YELLOW && pixel != display::BLACK) {
            if (pixel == display::RED && state == STATE_CONTINUING_FROM_STOP_LINE) {
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
    for (int y_idx = y; y_idx >= y-10; y_idx--) {
        int x = round(x_double);

        assert(y_idx >= 0 && y_idx < MAX_VIEW_HEIGHT);
        assert(x-5 >= 0 && x+5 < MAX_VIEW_WIDTH);

        for (int x_idx = x-5; x_idx <= x+5; x_idx++) {
            if (view[y_idx][x_idx] == display::GREEN) {
                return true;
            }
        }
        x_double += slope;
    }

    return false;
}

void autonomous_car::scan_ahead_for_minigap(view_t &view, int y, double x, double slope,
                                            int &minigap_y_last, double &minigap_slope, string &minigap_type_str)
{
    minigap_y_last = NO_VALUE;
    minigap_slope = 0;
    minigap_type_str = "NONE";

    assert(y-4 >= 0 && y-1 < MAX_VIEW_HEIGHT);

    if (fullgap.valid) {
        for (int y_idx = y-1; y_idx >= y-4; y_idx--) {
            if (y_idx < fullgap.y_start_view && y_idx > fullgap.y_end_view) {

                double deltay = (fullgap.y_start_view - y_idx);
                double fullgap_slope = -(fullgap.x_end_view - fullgap.x_start_view) / 
                                        (fullgap.y_end_view - fullgap.y_start_view);
                double x_fullgap = fullgap.x_start_view + deltay * fullgap_slope;

                minigap_y_last = y_idx+1;
                minigap_slope = -(x_fullgap - (x-slope)) / (y_idx - (y+1));
                minigap_type_str = "PRECEED_FULLGAP";
                return;
            }
        }
    }

    for (int y_idx = y-1; y_idx >= y-4; y_idx--) {
        double x_scl;
        x_scl = scan_across_for_center_line(view, y_idx, x + (y - y_idx) * slope);
        if (x_scl != NO_VALUE) {
            minigap_y_last = y_idx+1;
            minigap_slope = -(x_scl - (x-slope)) / (y_idx - (y+1));
            minigap_type_str = "CENTERLINE_GAP";
            return;
        }
    }
}

void autonomous_car::scan_ahead_for_continuing_center_lines(
            view_t &view, int y_start, int x_start, double slope_start,
            int &y_straight, int &x_straight, 
            int &y_left, int &x_left,
            int &y_right, int &x_right)
{
    struct {
        int y_first;
        int x_first;
        int y_last;
        int x_last;
    } found_line[3];
    int max_found_line = 0;
    int gen_xy_phase, x, y, r, i;

    // preset returns
    y_straight = NO_VALUE;
    x_straight = NO_VALUE;
    y_left = NO_VALUE;
    x_left = NO_VALUE;
    y_right = NO_VALUE;
    x_right = NO_VALUE;

    // loop over expanding perimetrs
    for (r = 7; r < 40; r++) {
        gen_xy_phase = 0;
        y = y_start - r;
        x = x_start;

        while (true) {
            // generate x,y
            if (gen_xy_phase == 0) {
                x--;
                if (x == x_start - r) {
                    gen_xy_phase++;
                }
            } else if (gen_xy_phase == 1) {
                y++;
                if (y == y_start + 1) {
                    y = y_start - r;
                    x = x_start;
                    gen_xy_phase++;
                }
            } else if (gen_xy_phase == 2) {
                x++;
                if (x == x_start + r) {
                    gen_xy_phase++;
                }
            } else if (gen_xy_phase == 3) {
                y++;
                if (y == y_start + 1) {
                    break;
                }
            }

            // if x or y out of range then continue
            if (x < 0 || x >= MAX_VIEW_WIDTH || y < 0 || y >= MAX_VIEW_HEIGHT) {
                continue;
            }

            // if view[y][x] not yellow then conitinue
            if (view[y][x] != display::YELLOW) {
                continue;
            }

            // if this y,x is a continuation of a found line then
            //    update the end of the found line
            //    continue
            // endif
            for (i = 0; i < max_found_line; i++) {
                if (abs(x-found_line[i].x_last) <= 3 && abs(y-found_line[i].y_last) <= 3) {
                    found_line[i].y_last = y;
                    found_line[i].x_last = x;
                    break;
                }
            }
            if (i < max_found_line) {
                continue;
            }
                    
            // found a new line, add it
            found_line[max_found_line].y_first = y;
            found_line[max_found_line].x_first = x;
            found_line[max_found_line].y_last = y;
            found_line[max_found_line].x_last = x;
            max_found_line++;

            // if found 3 lines then we're done
            if (max_found_line == 3) {
                break;
            }
        }

        // if found 3 lines then we're done
        if (max_found_line == 3) {
            break;
        }
    }

    // determine the angle of the incoming center line
    double angle_start;
    angle_start = atan(slope_start) * (180./M_PI);

    // loop over the continuing center lines that have been found
    //  - determine the continuing center line angle
    //  - compare the continuing center line angle with the incomoing center line angle,
    //    and categorize the continuing center line as either a left turn, right turn, or 
    //    straight continuation
    // endloop
    DEBUG_ID("cont_line - search started at" << 
             " y,x " << y_start << " " << x_start <<
             " angle_start " << angle_start <<
             endl);
    for (int i = 0; i < max_found_line; i++) {
        double slope, angle;
        string direction;

        slope = -(double)(found_line[i].x_first - (x_start-slope_start)) / (found_line[i].y_first - (y_start+1));
        angle = atan(slope) * (180./M_PI);

        if (angle - angle_start < -20) { 
            y_left = found_line[i].y_first;
            x_left = found_line[i].x_first;
            direction = "left";
        } else if (angle - angle_start > 20) { 
            y_right = found_line[i].y_first;
            x_right = found_line[i].x_first;
            direction = "right";
        } else {
            y_straight = found_line[i].y_first;
            x_straight = found_line[i].x_first;
            direction = "straight";
        }

        DEBUG_ID("cont_line - " << direction << 
                 " y,x " << found_line[i].y_first << " " << found_line[i].x_first <<
                 " angle-to-cont-line " << angle-angle_start <<
                 endl);
    }
}

void autonomous_car::set_car_controls()
{
    assert(distance_road_is_clear != NO_VALUE);

    // ---------------------
    // steering control ...
    // ---------------------

    // controls how far ahead is the point that the car is being steered toward
    const double K_STEER_DISTANCE_FACTOR = 0.5;

    // calculate steer_ctl_val, which is the direction of the front wheels in degrees
    double steer_ctl_val;
    if (get_speed() == 0) {
        steer_ctl_val = 0;
        DEBUG_ID("steer - direction " << steer_ctl_val << ", speed is zero" << endl);
    } else if (distance_road_is_clear <= 5) {
        steer_ctl_val = get_steer_ctl();
        DEBUG_ID("steer - direction " << steer_ctl_val << ", low distance_road_is_clear" << endl);
    } else {
        int steer_distance = get_speed() * K_STEER_DISTANCE_FACTOR;
        if (steer_distance < 10) {
            steer_distance = 10;
        } 
        if (steer_distance >= distance_road_is_clear) {
            steer_distance = distance_road_is_clear - 1;
        }
        steer_ctl_val = atan((x_line[steer_distance] + 7 - xo) / steer_distance) * (180./M_PI);
        DEBUG_ID("steer - direction " << steer_ctl_val << ", steer_distance " << steer_distance << 
                 " road_clear " << distance_road_is_clear << " speed " << get_speed() << endl);
    }

    // limit steer_ctl_val to allowed range
    if (steer_ctl_val < MIN_STEER_CTL) {
        steer_ctl_val = MIN_STEER_CTL;
    } else if (steer_ctl_val > MAX_STEER_CTL) {
        steer_ctl_val = MAX_STEER_CTL;
    }

    // set steering control
    set_steer_ctl(steer_ctl_val);

    // ---------------------
    // speed control ...
    // ---------------------

    // normal braking, for example to come to stop at a stop line
    const double K_DESIRED_DECEL = MIN_SPEED_CTL * 0.33;  // mph per second
    // control how quickly the car accelerates to a desired speed
    const double K_ACCEL_FACTOR  = 0.5;

    // determine adjusted_distance_road_is_clear, which is a slightly reduced
    // distance_road_is_clear; this is used in the calculations below so
    // that the car will stop a few feet before the obstruction
    double adjusted_distance_road_is_clear;
    adjusted_distance_road_is_clear = (double)(distance_road_is_clear-5) / 5280; 
    if (adjusted_distance_road_is_clear < 0.1 / 5280) {
        adjusted_distance_road_is_clear = 0.1 / 5280;
    }

    // calculate the deceleration needed to stop the car in adjusted_distance_road_is_clear
    // . A = -V^2 / (2*S)
    double decel_needed;
    decel_needed = -(get_speed() * get_speed()) / (2 * adjusted_distance_road_is_clear);
    decel_needed /= 3600;  // mph per second

    // calculate the speed_ctl_val, which is the desired acceleration (negative for decel) in mph/sec
    //
    // if the deceleration needed to stop before the obstruction is larger than the K_DESIRED_DECEL 
    // then 
    //   decelerate at a rate needed to stop in time
    // else
    //   determine desired_speed as the max speed that would allow the car to stop when 
    //    decleration is K_DESIRED_DECEL  (V = sqrt(2*A*S))
    //   accelerate at a rate proportional to the difference between current and desired speed
    // endif
    double speed_ctl_val;
    if (decel_needed < K_DESIRED_DECEL) {
        speed_ctl_val = decel_needed;   // mph per second
        DEBUG_ID("decel - speed_ctl_val " << speed_ctl_val 
                 << " decel_needed " << decel_needed << " road_clear " << adjusted_distance_road_is_clear*5280
                 << endl);
    } else {
        double desired_speed;
        if (adjusted_distance_road_is_clear == 0.1 / 5280) {
            desired_speed = 0;
        } else {
            desired_speed = sqrt(2 * (-K_DESIRED_DECEL*3600) * adjusted_distance_road_is_clear);
            if (desired_speed > get_max_speed()) {
                desired_speed = get_max_speed();
            }
        }

        speed_ctl_val = (desired_speed - get_speed()) * K_ACCEL_FACTOR;  // mph per second

        DEBUG_ID("accel - speed_ctl_val " << speed_ctl_val 
                 << " decel_needed " << decel_needed << " road_clear " << adjusted_distance_road_is_clear*5280
                 << " desired_speed " << desired_speed << " current_speed " << get_speed() 
                 << endl);
    }

    // limit speed_ctl_val to allowed range
    if (speed_ctl_val < MIN_SPEED_CTL) {
        speed_ctl_val = MIN_SPEED_CTL;
    } else if (speed_ctl_val > MAX_SPEED_CTL) {
        speed_ctl_val = MAX_SPEED_CTL;
    }

    // set the speed control
    set_speed_ctl(speed_ctl_val);
}

// -----------------  MISC SUPPORT --------------------------------------------------

void autonomous_car::state_change(enum state new_state)
{
    DEBUG_ID("state_change - " << state_string(state) << " -> " << state_string(new_state) << endl);
    state = new_state;
    time_in_this_state_us = 0;
}

const string autonomous_car::state_string(enum state s)
{
    switch (s) {
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
    case STATE_CONTINUING_FROM_STOP_LINE:
        return "CONTINUING_FROM_STOP_LINE";
    default:
        return "INVALID";
    }
}

const string autonomous_car::obstruction_string(enum obstruction o)
{
    switch (o) {
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

void autonomous_car::coord_convert_view_to_fixed(double y_view, double x_view, double &y_fixed, double &x_fixed)
{
    double cosine = cos(-get_dir() * (M_PI/180));
    double sine   = sin(-get_dir() * (M_PI/180));
    double x_view_rotated, y_view_rotated;

    y_view = MAX_VIEW_HEIGHT-1 - y_view;
    x_view = x_view - MAX_VIEW_WIDTH/2;

    x_view_rotated = x_view * cosine - y_view * sine;
    y_view_rotated = x_view * sine + y_view * cosine;

    x_fixed = get_x() + x_view_rotated;
    y_fixed = get_y() - y_view_rotated;
}

void autonomous_car::coord_convert_fixed_to_view(double y_fixed, double x_fixed, double &y_view, double &x_view)
{
    double cosine = cos(get_dir() * (M_PI/180));
    double sine   = sin(get_dir() * (M_PI/180));
    double x_view_rotated, y_view_rotated;

    x_view_rotated = x_fixed - get_x();
    y_view_rotated = -(y_fixed - get_y());

    x_view = x_view_rotated * cosine - y_view_rotated * sine;
    y_view = x_view_rotated * sine + y_view_rotated * cosine;

    y_view = MAX_VIEW_HEIGHT-1 - y_view;
    x_view = x_view + MAX_VIEW_WIDTH/2;
}

