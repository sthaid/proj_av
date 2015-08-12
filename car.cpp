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

// -----------------  SET CAR CONTROLS  ---------------------------------------------

void car::set_speed_ctl(double v)
{
    speed_ctl = v;
}

void car::set_steer_ctl(double v)
{
    steer_ctl = v;
}

// -----------------  UPDATE CAR POSITION, DIR, SPEED--------------------------------

void car::update(double microsecs)
{
    double distance;
    // xxx very preliminary

    distance = speed * microsecs * (5280./3600./1e6);

    x += distance * cos((dir+270.) * (M_PI/180.0));
    y += distance * sin((dir+270.) * (M_PI/180.0));
}

