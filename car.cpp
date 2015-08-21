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
    // front view
    unsigned char front_view[MAX_FRONT_VIEW_XY*MAX_FRONT_VIEW_XY];
    w.get_view(x, y, dir, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view);
    d.texture_set_rect(texture, 0, 0, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view, MAX_FRONT_VIEW_XY);
    d.texture_draw(texture, 0, 0, MAX_FRONT_VIEW_XY, MAX_FRONT_VIEW_XY, front_view_pid);

    // dashboard  (600 x 100)
    // xxx should be able to get the pane w,h if desired from display.h

    // current speed
    // XXX put text at exact pixel
    std::ostringstream s;
    s << speed;
    d.text_draw(s.str(), 0, 0, dashboard_pid);

    // steering control
    int x;
    d.draw_set_color(display::WHITE);
    d.draw_rect(150,25,300,50,dashboard_pid,2);

    x = 300;
    d.draw_line(x-1, 20, x-1, 24, dashboard_pid);
    d.draw_line(x+0, 20, x+0, 24, dashboard_pid);
    d.draw_line(x+1, 20, x+1, 24, dashboard_pid);

    d.draw_line(x-1, 75, x-1, 79, dashboard_pid);
    d.draw_line(x+0, 75, x+0, 79, dashboard_pid);
    d.draw_line(x+1, 75, x+1, 79, dashboard_pid);

    d.draw_set_color(display::ORANGE);
    x = 152 + (steer_ctl - MIN_STEER_CTL) / (MAX_STEER_CTL - MIN_STEER_CTL) * 296;
    if (x < 152) x = 152;
    if (x > 447) x = 447;
    d.draw_line(x-1, 27, x-1, 72, dashboard_pid);
    d.draw_line(x+0, 27, x+0, 72, dashboard_pid);
    d.draw_line(x+1, 27, x+1, 72, dashboard_pid);

    // speed control - brake
    d.draw_set_color(display::WHITE);
    d.draw_rect(470,0,50,100,dashboard_pid,2);
    if (speed_ctl < 0) {
        d.draw_set_color(display::RED);
        int height = (speed_ctl / MIN_SPEED_CTL) * 96 + 0.5;
        if (height < 1) height = 1;
        if (height > 96) height = 96;
        d.draw_filled_rect(472, 98-height, 46, height, dashboard_pid);
    }

    // speed control - gas  
    d.draw_set_color(display::WHITE);
    d.draw_rect(540,0,50,100,dashboard_pid,2);
    if (speed_ctl > 0) {
        d.draw_set_color(display::GREEN);
        int height = (speed_ctl / MAX_SPEED_CTL) * 96 + 0.5;
        if (height < 1) height = 1;
        if (height > 96) height = 96;
        d.draw_filled_rect(542, 98-height, 46, height, dashboard_pid);
    }
}

// -----------------  UPDATE CAR CONTROLS  ------------------------------------------

void car::set_steer_ctl(double val) 
{
    if (val > MAX_STEER_CTL) {
        steer_ctl = MAX_STEER_CTL;
    } else if (val < MIN_STEER_CTL) {
        steer_ctl = MIN_STEER_CTL;
    } else {
        steer_ctl = val;
    }
}

void car::set_speed_ctl(double val) 
{
    if (speed_ctl > MAX_SPEED_CTL) {
        speed_ctl = MAX_SPEED_CTL;
    } else if (speed_ctl < MIN_SPEED_CTL) {
        speed_ctl = MIN_SPEED_CTL;
    } else {
        speed_ctl = val;
    }
}

// -----------------  UPDATE CAR POSITION, DIR, SPEED--------------------------------

// units:
// - distance:  feet
// - speed:     mph
// - dir:       degrees
// - steer_ctl: degrees
// - speed_ctl: mph/sec

void car::update_mechanics(double microsecs)
{
    double distance;
    double delta_dir;

    // update car position based on current direction and speed
    distance = speed * microsecs * (5280./3600./1e6);
    x += distance * cos((dir+270.) * (M_PI/180.0));
    y += distance * sin((dir+270.) * (M_PI/180.0));

    // update car direction based upon steering control 
    delta_dir = atan(distance / CAR_LENGTH * sin(steer_ctl*(M_PI/180.))) * (180./M_PI);
    dir += delta_dir;
    if (dir < 0) {  // xxx look at everywhere 360 is used, make an inline util for this
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

