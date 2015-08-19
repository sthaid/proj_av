#include "fixed_control_car.h"
#include "logging.h"
#include "utils.h"

fixed_control_car::fixed_control_car(display &display, world &world, double x, double y, double dir, double speed)
    : car(display,world,x,y,dir,speed)
{
}

fixed_control_car::~fixed_control_car()
{
}

void fixed_control_car::update_controls(double microsecs)
{
    // get front_view, this is done to just get an idea of the processing time
    unsigned char front_view[200*200];
    world &w = get_world();
    w.get_view(get_x(), get_y(), get_dir(), 200, 200, reinterpret_cast<unsigned char *>(front_view));
}

