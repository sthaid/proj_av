#include <sstream>
#include <cassert>
#include <math.h>
#include <memory.h>

#include "car.h"
#include "logging.h"
#include "utils.h"

using std::ostringstream;

// -----------------  CAR CLASS STATIC INITIALIZATION  ------------------------------

display::texture *car::texture;

void car::static_init(display &d)
{
    // xxx should be destroyed 
    texture = d.texture_create(MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY);
    assert(texture);
}

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

car::car(display &display, world &world, double x_arg, double y_arg, double dir_arg, double speed_arg) 
    : d(display), w(world)
{
    x         = x_arg;
    y         = y_arg;
    dir       = dir_arg;
    speed     = speed_arg;
    speed_ctl = 0;
    steer_ctl = 0;
}

car::~car()
{
}

// -----------------  DRAW CAR STATE  -----------------------------------------------

void car::draw(int front_view_pid, int dashboard_pid)
{
    unsigned char front_view[MAX_FRONT_VIEW_XY*MAX_FRONT_VIEW_XY];
    w.get_view(x, y, dir, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view);
    d.texture_set_rect(texture, 0, 0, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view, MAX_FRONT_VIEW_XY);
    d.texture_draw(texture, 0, 0, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view_pid);

    std::ostringstream s;
    s << speed << " MPH";
    d.text_draw(s.str(), 0, 0, dashboard_pid, false, 0, 0, true);
    d.text_draw("dashboard line 2", 1, 0, dashboard_pid);
}

// -----------------  UPDATE CAR CONTROLS  ------------------------------------------

void car::set_steer_ctl(double val) 
{
    steer_ctl = val;
}

void car::set_speed_ctl(double val) 
{
    speed_ctl = val;
}

// -----------------  UPDATE CAR POSITION, DIR, SPEED--------------------------------

// units:
// - distance:  feet
// - speed:     mph
// - dir:       degrees
// - steer_ctl: degrees/sec
// - speed_ctl: mph/sec

// xxx preliminary
void car::update_mechanics(double microsecs)
{
    const double MAX_SPEED = 30;  // mph

    double distance;

    // XXX max steer_ctl and speed_ctl needed

    // update car position based on current direction and speed
    distance = speed * microsecs * (5280./3600./1e6);
    x += distance * cos((dir+270.) * (M_PI/180.0));
    y += distance * sin((dir+270.) * (M_PI/180.0));

    // update car direction based upon steering control 
    dir += steer_ctl * (microsecs / 1000000.);
    if (dir < 0) {
        dir += 360;
    } else if (dir > 360) {
        dir -= 360;
    }
    
    // update car speed based on speed control 
    speed += speed_ctl * (microsecs / 1000000.);
    if (speed < 0) {
        speed = 0;
    } else if (speed > MAX_SPEED) {
        speed = MAX_SPEED;
    }
}

