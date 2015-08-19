#include <math.h>

#include "autonomous_car.h"
#include "logging.h"
#include "utils.h"

autonomous_car::autonomous_car(display &display, world &world, double x, double y, double dir, double speed)
    : car(display,world,x,y,dir,speed)
{
}

autonomous_car::~autonomous_car()
{
}

void autonomous_car::update_controls(double microsecs)
{
    // get front_view, this is done to just get an idea of the processing tie
    unsigned char front_view[200*200];
    world &w = get_world();
    w.get_view(get_x(), get_y(), get_dir(), 200, 200, reinterpret_cast<unsigned char *>(front_view));

    //set_steer_ctl(get_dir() < 90 ? 10 : 0);
    //set_speed_ctl(get_speed() < 20 ? 10 : 0);
}

