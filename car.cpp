#include <math.h>

#include "car.h"
#include "logging.h"
#include "utils.h"

// -----------------  CONSTRUCTOR / DESTRUCTOR  -------------------------------------

car::car(double x_arg, double y_arg, double dir_arg, double speed_arg)
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

// xxx very preliminary
void car::update_mechanics(double microsecs)
{
    const double MAX_SPEED = 30;  // mph

    double distance;

    // units:
    // - distance:  feet
    // - speed:     mph
    // - dir:       degrees
    // - steer_ctl: degrees/sec
    // - speed_ctl: mph/sec

    // XXX max steer_ctl and speed_ctl needed

    distance = speed * microsecs * (5280./3600./1e6);
    x += distance * cos((dir+270.) * (M_PI/180.0));
    y += distance * sin((dir+270.) * (M_PI/180.0));

    dir += steer_ctl * (microsecs / 1000000.);
    if (dir < 0) {
        dir += 360;
    } else if (dir > 360) {
        dir -= 360;
    }
    
    speed += speed_ctl * (microsecs / 1000000.);
    if (speed < 0) {
        speed = 0;
    } else if (speed > MAX_SPEED) {
        speed = MAX_SPEED;
    }
}

