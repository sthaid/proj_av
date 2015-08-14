#include <math.h>

#include "car.h"
#include "logging.h"
#include "utils.h"

fixed_control_car::fixed_control_car(double x, double y, double dir, double speed)
    : car(x,y,dir,speed)
{
}

fixed_control_car::~fixed_control_car()
{
}

void fixed_control_car::update_controls(double microsecs)
{
    //set_steer_ctl(get_dir() < 90 ? 10 : 0);
    //set_speed_ctl(get_speed() < 20 ? 10 : 0);
}

