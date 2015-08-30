#include <cassert>
#include <iomanip>
#include <math.h>

#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

const int MAX_VIEW_WIDTH = 151;
const int MAX_VIEW_HEIGHT = 390;

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

autonomous_car::autonomous_car(display &display, world &world, int id, double x, double y, double dir, double speed)
    : car(display,world,id,x,y,dir,speed)
{
}

autonomous_car::~autonomous_car()
{
}

// -----------------  DRAW VIEW VIRTUAL FUNCTION  ---------------------------------

void autonomous_car::draw_view(int pid)
{
    static struct display::texture * t;
    unsigned char view[MAX_VIEW_WIDTH*MAX_VIEW_HEIGHT];
    class display &d = get_display();
    class world &w = get_world();

    if (t == NULL) {
        t = d.texture_create(MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);
        assert(t);
    }

    w.get_view(get_x(), get_y(), get_dir(), MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, view);
    d.texture_set_rect(t, 0, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, view, MAX_VIEW_WIDTH);
    d.texture_draw2(t, pid, 300-MAX_VIEW_WIDTH/2, 0, MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT);

    d.text_draw("FRONT", 0, 0, pid, false, 0, 1, true);
}

// -----------------  DRAW DASHBOARD VIRTUAL FUNCTION  -----------------------------

void autonomous_car::draw_dashboard(int pid)
{
    class display &d = get_display();

    // call base class draw_dashboard 
    // XXX show failed in car.cpp dash
    car::draw_dashboard(pid);

    // draw around the autonomous dashboard
    d.draw_rect(0,98,590,102,pid,2);

    // XXX autonomous dash content is tbd
    d.text_draw("AUTONOMOUS DASH", 3.0, 0, pid, false, 0, 0, true);
}

// -----------------  UPDATE CONTROLS VIRTUAL FUNCTION  -----------------------------

// NOTES
// - lane width   = 12 ft
// - car width    =  7 ft
// - car length   = 15 ft
//
// XXX should make the road 1 ft wider so car can be centered,
//     as it is now:   line : 2ft rd : 7ft car : 3ft rd

void autonomous_car::update_controls(double microsecs)
{
    const int NO_VALUE = 999999;

    // XXX review this entire routine for numeric constants

    // XXX supersection comments

    // XXX review use of floating point

    // XXX this routine is becoming too long

    //
    // if car has failed then return
    //

    if (get_failed()) {
        return;
    }

    INFO("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx\n");

    //
    // get front_view, this is done to just get an idea of the processing tie
    //

    unsigned char fv[MAX_VIEW_HEIGHT][MAX_VIEW_WIDTH];
    world &w = get_world();
    w.get_view(get_x(), get_y(), get_dir(), MAX_VIEW_WIDTH, MAX_VIEW_HEIGHT, reinterpret_cast<unsigned char *>(fv));

    //
    // coords of the center of the car
    //

    int xo = MAX_VIEW_WIDTH/2;
    int yo = MAX_VIEW_HEIGHT-1;
    INFO("xo,yo " << xo << " " << yo << endl);

    //
    // create table of the x coord of the center line, indexed by y;
    // table entries will be skipped (for now) if there is no center line found at the y coord
    //

    double xl_tbl[1000];
    int min_yl = NO_VALUE;
    double xl = xo - 6;
    double slopel = 0;

    for (auto& xl_tbl_ent : xl_tbl) {
        xl_tbl_ent = NO_VALUE;
    }

    for (int yl = yo; yl >= 0; yl--) {
        INFO("*** YL " << yl << " XL " << xl << endl);

        // if xl is too close to the edge of the front view then 
        // exit this loop
        if (xl < 10 || xl >= MAX_VIEW_WIDTH-10) {
            INFO("BREAK XL " << xl << endl);
            break;
        }

        // scan for center line at yl in xl range;
        int xl_found_start = NO_VALUE;
        int xl_found_end   = NO_VALUE;
        INFO("scan " << xl-8 << " to " << xl+8 << endl);
        for (int xl_scan = xl-8; xl_scan <= xl+8; xl_scan++) {
            if (fv[yl][xl_scan] == display::YELLOW) {
                if (xl_found_start == NO_VALUE) {
                    xl_found_start = xl_scan;
                }
                xl_found_end = xl_scan;
            }
        }
        INFO("xl_found_start,end = " << xl_found_start << " " << xl_found_end << endl);

        // if center line is found ....
        if (xl_found_start != NO_VALUE) {
            // center line is found ...
            //
            // update center line x coord
            xl = (double)(xl_found_start + xl_found_end) / 2;

            // save center line x coord in the xl_tbl
            xl_tbl[yl] = xl;
            min_yl = yl;
            INFO("xl_tbl[y] = " << xl_tbl[yl] << " min_yl = " << min_yl << endl);

            // if possible, update the slope of the center line;
            // the slope is delta-x / delta-y
            for (int delta_y = 10; delta_y <= 12; delta_y++) {
                if (xl_tbl[yl+delta_y] != NO_VALUE) {
                    double delta_x = xl - xl_tbl[yl+delta_y];
                    slopel = delta_x / delta_y;
                    INFO("FOUND slopel = " << slopel << endl);
                    break;
                }
            }
        } else {
            // center line is not found ...
            //
            // update center line x coord
            xl += slopel;
            INFO("NOT FOUND slopel = " << slopel << endl);
        }
    }

    //
    // fill in missing xl_tbl entries
    //

    // search the xl_tbl alongside the car to see if center line has been found 
    // anywhere alongside the car; fill in the centerline location alongside the car
    // with either the value that was found or (when no value was found) with the
    // expected centerline location
    double value = xo - 6;;
    for (int i = 7; i >= 0; i--) {
        if (xl_tbl[yo-i] != NO_VALUE) {
            value = xl_tbl[yo-i];
            break;
        }
    }
    for (int i = 7; i >= 0; i--) {
        xl_tbl[yo-i] = value;
    }
    if (min_yl == NO_VALUE || min_yl > yo-7) {
        min_yl = yo-7;
    }

    INFO("xl_tbl \n");
    for (int yl = yo; yl >= min_yl; yl--) {
        INFO(yl << " " << xl_tbl[yl] << endl);
    }

    // loop over the table 
    // - check for gap too long, and
    // - fill in gaps using linear interpolation
    int last_valid_yl = yo;
    for (int yl = yo-1; yl >= min_yl; yl--) {
        int gap_length = last_valid_yl - yl - 1;
        if (gap_length > 35) {
            INFO("FAILED - center line gap too long" << endl);
            set_failed();
            return;
        }

        if (xl_tbl[yl] != NO_VALUE) {
            for (int yl_idx = last_valid_yl-1; yl_idx > yl; yl_idx--) {
                xl_tbl[yl_idx] = 
                    xl_tbl[last_valid_yl] +
                    (xl_tbl[yl] - xl_tbl[last_valid_yl]) / (last_valid_yl - yl) * (last_valid_yl - yl_idx);
            }
            last_valid_yl = yl;
        }
    }

    //
    // if there is no center line 30 feet in front of car then fail
    //

    if (xl_tbl[yo - 7 - 30] == NO_VALUE) {
        INFO("FAILED - no center line 30 feet up the road" << endl);
        set_failed();
        return;
    }

    //
    // determine the angle to the location of the center line DIST feet up the road
    //

    double delta_x, delta_y, angle;
    delta_x = xl_tbl[yo-37]+6 - MAX_VIEW_WIDTH/2;
    delta_y = 37;
    angle = atan(delta_x/delta_y) * (180./M_PI);
    INFO("angle " << angle << " deltax,y " << delta_x << " " << delta_y << endl);

    // set steering control
    set_steer_ctl(angle);

    //
    // determine clear distance 
    // XXX improve this
    //

    double distance_road_is_clear;
    int count = 0;
    for (int yl = yo-8; yl >= min_yl; yl--) {
        if (fv[yl][(int)xl_tbl[yl]+6] == display::RED) {   // xxx use round
            if (get_speed()) {
                INFO("RED: DIST " << count << endl);
            }
            break;
        }
        count++;
    }
    distance_road_is_clear = (double)count / 5280.;

    //
    // speed control
    //

    double speed_target;
    double current_speed;
    double speed_ctl_val;
    double adjusted_distance_road_is_clear;
    const double K_ACCEL = 1.5;
    const double K_DECEL = 5.0;

    adjusted_distance_road_is_clear = distance_road_is_clear - 5./5280;
    if (adjusted_distance_road_is_clear < 0) {
        adjusted_distance_road_is_clear = 0;
    }

    speed_target = sqrt(2. * (-MIN_SPEED_CTL*3600/4) * adjusted_distance_road_is_clear);   // mph
    if (speed_target > MAX_SPEED) {
        speed_target = MAX_SPEED;
    }

    current_speed = get_speed();

    if (speed_target >= current_speed) {
        speed_ctl_val = (speed_target - current_speed) * K_ACCEL;
    } else {
        speed_ctl_val = (speed_target - current_speed) * K_DECEL;
    }

    if (current_speed > 0) {
        INFO("DIST " << std::setprecision(2) << adjusted_distance_road_is_clear*5280 << " SPEED_TGT,CUR " << speed_target << " " << current_speed << " SPEED_CTL " << speed_ctl_val << endl);
    }

    set_speed_ctl(speed_ctl_val);



// XXX clean up this code
// XXX adjust speed based on clear distance 
}

