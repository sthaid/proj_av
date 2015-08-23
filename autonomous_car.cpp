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

#if 0
locate yellow line in front left of car, up to N feet away, scanning in 
the current direction of the car

if yellow line is not found then fail the car

set steering and speed based on the characteristics of the yellow line

check the road ahead for obstructions, including
- other vehicle
- stop line
- dead end road
and adjust speed accordingly

if car has come to a stop at a stop line (only T intersecions supported)
  if turn is not chosen then 
    choose left or right turn at random
    locate cross road yellow line in both directions
  endif

  if right turn and no approaching cars from the left
    initiate right turn
  endif

  if left turn and no approaching cars from the left or right
    initiate left turn
  endif
endif

todo
- display failed cars in red
- status counters for the number of failed and good cars

#endif
}

